/*
 * Copyright (c) 2013, Roland Bock
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TabSample.h"
#include <sqlpp11/alias_provider.h>
#include <sqlpp11/select.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/update.h>
#include <sqlpp11/remove.h>
#include <sqlpp11/functions.h>
#include <sqlpp11/transaction.h>
#include <sqlpp11/multi_column.h>
#include <sqlpp11/mysql/connection.h>

#include <iostream>
#include <vector>


SQLPP_ALIAS_PROVIDER(left);

namespace mysql = sqlpp::mysql;
int main()
{
	auto config = std::make_shared<mysql::connection_config>();
 	config->user = "root";
 	config->database = "sqlpp_mysql";
	config->debug = true;
	try
	{
		mysql::connection db(config);
	}
	catch(const sqlpp::exception& )
	{
		std::cerr << "For testing, you'll need to create a database sqlpp_mysql with a table tab_sample, as shown in tests/TabSample.sql" << std::endl;
		throw;
	}
	mysql::connection db(config);
	db.execute(R"(DROP TABLE IF EXISTS tab_sample)");
	db.execute(R"(CREATE TABLE tab_sample (
		alpha bigint(20) DEFAULT NULL,
			beta bool DEFAULT NULL,
			gamma varchar(255) DEFAULT NULL
			))");
	db.execute(R"(DROP TABLE IF EXISTS tab_foo)");
	db.execute(R"(CREATE TABLE tab_foo (
		omega bigint(20) DEFAULT NULL
			))");


	TabSample tab;

	// explicit all_of(tab)
	for(const auto& row : db.run(select(all_of(tab)).from(tab).where(true)))
	{
		std::cerr << "row.alpha: " << row.alpha << ", row.beta: " << row.beta << ", row.gamma: " << row.gamma <<  std::endl;
	};
	// selecting a table implicitly expands to all_of(tab)
	for(const auto& row : db.run(select(all_of(tab)).from(tab).where(true)))
	{
		std::cerr << "row.alpha: " << row.alpha << ", row.beta: " << row.beta << ", row.gamma: " << row.gamma <<  std::endl;
	};
	// selecting two multicolumns
	for(const auto& row : db.run(select(multi_column(left, tab.alpha, tab.beta, tab.gamma), multi_column(tab, all_of(tab))).from(tab).where(true)))
	{
		std::cerr << "row.left.alpha: " << row.left.alpha << ", row.left.beta: " << row.left.beta << ", row.left.gamma: " << row.left.gamma <<  std::endl;
		std::cerr << "row.tabSample.alpha: " << row.tabSample.alpha << ", row.tabSample.beta: " << row.tabSample.beta << ", row.tabSample.gamma: " << row.tabSample.gamma <<  std::endl;
	};

	// test functions and operators
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.is_null()));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.is_not_null()));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.in(1, 2, 3)));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.in(sqlpp::value_list(std::vector<int>{1, 2, 3, 4}))));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.not_in(1, 2, 3)));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha.not_in(sqlpp::value_list(std::vector<int>{1, 2, 3, 4}))));
	db.run(select(count(tab.alpha)).from(tab).where(true));
	db.run(select(avg(tab.alpha)).from(tab).where(true));
	db.run(select(max(tab.alpha)).from(tab).where(true));
	db.run(select(min(tab.alpha)).from(tab).where(true));
	db.run(select(exists(select(tab.alpha).from(tab).where(tab.alpha > 7))).from(tab).where(true));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha == any(select(tab.alpha).from(tab).where(tab.alpha < 3))));
	db.run(select(all_of(tab)).from(tab).where(tab.alpha == some(select(tab.alpha).from(tab).where(tab.alpha < 3))));

	db.run(select(all_of(tab)).from(tab).where(tab.alpha + tab.alpha > 3));
	db.run(select(all_of(tab)).from(tab).where((tab.beta + tab.beta) == ""));
	db.run(select(all_of(tab)).from(tab).where((tab.beta + tab.beta).like("%'\"%")));
	db.run(select(all_of(tab)).from(sqlpp::verbatim_table("tab_sample")).where((tab.beta + tab.beta).like("%'\"%")));

	// insert
	db.run(insert_into(tab).set(tab.gamma = true));

	// update
	db.run(update(tab).set(tab.gamma = false).where(tab.alpha.in(1)));
	db.run(update(tab).set(tab.gamma = false).where(tab.alpha.in(sqlpp::value_list(std::vector<int>{1, 2, 3, 4}))));

	// remove
	db.run(remove_from(tab).where(tab.alpha == tab.alpha + 3));


	auto result = db.run(select(all_of(tab)).from(tab).where(true));
	std::cerr << "Accessing a field directly from the result (using the current row): " << result.begin()->alpha << std::endl;
	std::cerr << "Can do that again, no problem: " << result.begin()->alpha << std::endl;

	auto tx = start_transaction(db);
	if (const auto& row = *db.run(select(all_of(tab), select(max(tab.alpha)).from(tab)).from(tab).where(true)).begin())
	{
		int x = row.alpha;
		int a = row.max;
		std::cerr << "-----------------------------" << row.beta << std::endl;
	}
	tx.commit();

	return 0;
}
