// Standalone rule generation program
#include<stdio.h>
#include <stdlib.h>
#include<vector>
#include<iostream>
#include<cmath>
#include<unordered_map>
#include<list>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <algorithm>
#include "stdafx.h"

// namespace for ease
using namespace std;

// create structure to hold rule data
struct Rule {
	//max for expansion
	vector<int> rowForRuleFront;
	//min for expansion
	vector<int> rowForRuleBack;
	vector<int> colExpansionIndex; // represents the column-index where the expansion/rule happens (this is to retrieve the other attributes of the rule)
	//rowForRuleFront, rowForRuleBack, and colExpansionIndex are parallel arrays (or vectors, but if you have to research, then look up parallel arrays).
	int coverageCounter = 0;
	int classCovered = -1; // -1 is default value, but could potentially match a data's class.
	//this is used when combining rules. The two rules which are combined are formed into a new a rule, and the previous two rules are eventually deleted.
	bool markForErasal = false;
};

void clearRule(Rule& usedRule)
{
	usedRule.rowForRuleFront.clear();
	usedRule.rowForRuleBack.clear();
	usedRule.coverageCounter = 0;
	usedRule.classCovered = -1;
	usedRule.colExpansionIndex.clear();
}

bool compareByCoverage(const Rule& rule1, const Rule& rule2) {
	return rule1.coverageCounter > rule2.coverageCounter;
}

struct Expansion
{
	//this is the row, or the set in the data that is being expanded from
	int rowExpandedfrom;
	//These indexes assume the Expansion struct is being used in some sort of table--in this implementation it is an unordered map
	//these indexes represent the key of the table, but not any value in the expansion data itself
	//the values within this variable are parallel to those in expansionDirection
	//there should only be two values at maximum, and one value at minimum for expansionIndexExpandedTo and expansionDirection.
	//the front of the list represent an upward expansion, and the back of the list represents a downward epxansion
	list<int> expansionIndexExpandedTo;
	//these values represent the expansion direction (1: upward, -1: downard) from rowExpandedFrom -> the parallel row of the table[expansionIndexExpandedTo].rowExpandedFrom. 
	list<int> expansionDirection;
	//this is the column or attribute from which the expansion is taking place--there can only be column index per expansion (but a row/set can be apart of two expansions still)
	int columnIndex;
	//if the current expansion is already in a rule, then this should be checked true. By default, expansions are not part of any rule. 
	bool alreadyInARule = false;
	//this will be used later on to count how many cases are covered
};

std::vector<std::vector<int>> readCSV(std::string filename) {
	std::vector<std::vector<int>> data;
	std::ifstream file(filename);

	if (file.is_open()) {
		std::string line;
		// Skip the header line
		std::getline(file, line);

		while (std::getline(file, line)) {
			std::vector<int> row;
			std::stringstream ss(line);
			std::string value;

			// Skip the ID column
			std::getline(ss, value, ',');

			while (std::getline(ss, value, ',')) {
				row.push_back(std::stoi(value));
			}

			//row.erase(row.begin());
			data.push_back(row);
		}

		file.close();
	}

	return data;
}



//combines rules which have the possibility to be combined
//these things need to be tested in order for a rule to be combined:
// 
//1)If the rules have their expansion at the same spot and their expansions are some intersection of each other and if all of the clauses are the same
//	Example:
//	Consider the two rules below
//	R1:	1 <= X1 <= 2
//		X2 = 3
//		X3 = 4
// 
//	R2:	2 <= X1 <= 4
//		X2 = 3
//		X3 = 4
//	The two expanding clauses are in the same attribue, 0. They also intersect each other
//	The remaining clauses are also the same. Therefore these rules can be combined
//	R1: 1 <= X1 <= 4
//		X2 = 3
//		X3 = 4
//
//2)the rule's clauses which are not expanded should all be the same
//	In that case, the values that are not expanded which are sitting in the same spot as the opposing rule's expansion should fit within the expansion comparison
//	Example:
//	Consider the two rules below
//	R1:	X1 = 2
//		X2 = 3
//		3 <= X3 <= 5
//
//	R2:	1 <= X1 <= 3
//		X2 = 3
//		X3 = 4
//	The only clause which is not expanded is X2. For both Rules X2 is the same.
//	Where R1 has an expansion "3 <= X3 <= 5" R2 has "X3 = 4". 4 fits within R1's expansion.
//	Where R2 has an expansion "1 <= X1 <= 3" R1 has "X1 = 2". 2 fits within R2's expansion.
//	Therefore two rules can be combined into
//	R:	1 <= X1 <= 3
//		X2 = 3
//		3 <= X3 <= 5
// 


//first method
vector<Rule> combineOverlappingRules(vector<Rule> rules, vector<vector<int>> data)
{
	vector<Rule> combinedRules; // Vector to store the combined rules

	for (int i = 0; i < rules.size(); ++i)
	{
		//skip rules which have already been combined
		if (rules[i].markForErasal)
		{
			continue;
		}
		int compareRow = rules[i].rowForRuleFront[0];//either front or back should be able to be used here
		for (int j = i + 1; j < rules.size(); ++j)
		{
			//skip rules which have already been combined or which are from different classes
			if (rules[j].markForErasal || rules[i].classCovered != rules[j].classCovered)
			{
				continue;
			}
			int matchRow = rules[j].rowForRuleFront[0];//either front or back should be able to be used here
			bool goodRow = false;
			if (rules[i].colExpansionIndex[0] == rules[j].colExpansionIndex[0])
			{
				int colum = rules[i].colExpansionIndex[0]; //the "n" is a crime
				int compBackRow = rules[i].rowForRuleBack[0];
				int compFrontRow = rules[i].rowForRuleFront[0];
				int matchBackRow = rules[j].rowForRuleBack[0];
				int matchFrontRow = rules[j].rowForRuleFront[0];

				int compBackExp = data.at(compBackRow).at(colum);
				int compFrontExp = data.at(compFrontRow).at(colum);
				int matchBackExp = data.at(matchBackRow).at(colum);
				int matchFrontExp = data.at(matchFrontRow).at(colum);

				// Check if the expansions intersect
				if ((compBackExp >= matchBackExp && compBackExp <= matchFrontExp) || (compFrontExp >= matchBackExp && compBackExp <= matchFrontExp))
				{
					// Check if all other clauses are the same
					bool allClausesSame = true;
					for (int col = 0; col < data.at(0).size() - 1; col++)
					{
						if (col != rules[i].colExpansionIndex[0])//column index should be the same for both
						{
							int compData = data.at(compareRow).at(col);
							int matchData = data.at(matchRow).at(col);
							if (compData != matchData)
							{
								allClausesSame = false;
								break;
							}
						}
					}

					// If all clauses are the same, combine the rules
					if (allClausesSame)
					{
						//rules[i].markForErasal = true;
						rules[j].markForErasal = true;
						Rule newRule = Rule();
						//newRule.classCovered = rules[i].classCovered;
						//newRule.colExpansionIndex.push_back(rules[i].colExpansionIndex[0]);
						//newRule.coverageCounter = 0;
						rules[i].rowForRuleBack[0] = (compBackExp < matchBackExp ? rules[i].rowForRuleBack[0] : rules[j].rowForRuleBack[0]);
						rules[i].rowForRuleFront[0] = (compFrontExp > matchFrontExp ? rules[i].rowForRuleFront[0] : rules[j].rowForRuleFront[0]);
						//combinedRules.push_back(newRule);
						//break; // one combination is all we need
					}
				}
			}
		}
	}

	// Add non-marked rules to combinedRules
	for (const Rule& rule : rules)
	{
		if (!rule.markForErasal)
			combinedRules.push_back(rule);
	}

	return combinedRules;
}

//second method
vector<Rule> combineRulesGoodClauses(vector<Rule> rules, vector<vector<int>> data)
{
	vector<Rule> combinedRules; // Vector to store the combined rules

	for (int i = 0; i < rules.size(); ++i)
	{
		//skip rules which have already been combined
		if (rules[i].markForErasal)
		{
			continue;
		}
		int compareRow = rules[i].rowForRuleFront[0];//either front or back should work here
		for (int j = i + 1; j < rules.size(); ++j)
		{
			//skip rules which have already been combined or which are from different classes
			if (rules[j].markForErasal || rules[i].classCovered != rules[j].classCovered)
			{
				continue;
			}
			int matchRow = rules[j].rowForRuleFront[0];//either front or back should work here
			bool goodRow = false;
			if (rules[i].colExpansionIndex[0] != rules[j].colExpansionIndex[0])
			{
				for (int col = 0; col < data.at(0).size() - 1; col++)//"-1" since class is accounted for above
				{
					//if expanded attribute
					if (rules[i].colExpansionIndex[0] == col)
					{
						//if the val of the current row and col is not within the expanded bounds of the rule, then go the next row
						if (!(data.at(matchRow).at(col) >= data.at(rules[i].rowForRuleBack[0]).at(col) && data.at(matchRow).at(col) <= data.at(rules[i].rowForRuleFront[0]).at(col)))
						{
							goodRow = false;
							break;
						}
						goodRow = true;
					}
					else if (rules[j].colExpansionIndex[0] == col)
					{
						//if the val of the current row and col is not within the expanded bounds of the rule, then go the next row
						if (!(data.at(compareRow).at(col) >= data.at(rules[j].rowForRuleBack[0]).at(col) && data.at(compareRow).at(col) <= data.at(rules[j].rowForRuleFront[0]).at(col)))
						{
							goodRow = false;
							break;
						}
						goodRow = true;
					}
					else
					{
						//if the current row at the current column does not match the usable row at the current column, then the rule does not apply to that row
						//go to the next
						if (!(data.at(compareRow).at(col) == data.at(matchRow).at(col)))
						{
							goodRow = false;
							break;
						}
						goodRow = true;
					}
				}
				if (goodRow)
				{
					//rules[i].markForErasal = true;
					rules[j].markForErasal = true;
					//Rule newRule = Rule();
					//newRule.classCovered = rules[i].classCovered;
					rules[i].colExpansionIndex.push_back(rules[j].colExpansionIndex[0]);
					//newRule.colExpansionIndex.push_back(rules[j].colExpansionIndex[0]);
					//newRule.coverageCounter = 0;
					//newRule.rowForRuleBack.push_back(rules[i].rowForRuleBack[0]);
					rules[i].rowForRuleBack.push_back(rules[j].rowForRuleBack[0]);
					//newRule.rowForRuleFront.push_back(rules[i].rowForRuleFront[0]);
					rules[i].rowForRuleFront.push_back(rules[j].rowForRuleFront[0]);
					//combinedRules.push_back(newRule);
					//break;//one combination is all we need
				}
			}
		}
	}

	// Add non-marked rules to combinedRules
	for (const Rule& rule : rules)
	{
		if (!rule.markForErasal)
			combinedRules.push_back(rule);
	}

	return combinedRules;
}

vector<Rule> generalizeRules(vector<Rule> rules, vector<vector<int>> data, float lowPrecision)
{
	//generalize the rules:
	// 1) go through each attribute, test the data without it--if the coverage goes up and the precision of the rule does not go below lowPrecision, then good. 
	// take that attribute out of the rule
	// 2) do this for each rule, for each attribute. 
	//needed first:
	// code that tests precision
	// precision variable added to Rule struct
	return {};
}

int testRule(Rule rule, vector<vector<int>> data)
{
	vector<int> epxandedColIndexes = rule.colExpansionIndex;
	vector<int> minColVals;
	vector<int> maxColVals;
	for (int j = 0; j < epxandedColIndexes.size(); j++)//unfortunate brute force method where sometimes the value in rowForRuleBack is bugger than RowForRuleFront
	{
		int minColVal = data.at(rule.rowForRuleBack[j]).at(epxandedColIndexes[j]);
		int maxColVal = data.at(rule.rowForRuleFront[j]).at(epxandedColIndexes[j]);

		minColVals.push_back(minColVal);
		maxColVals.push_back(maxColVal);
	}

	//a usable row for accessing the rules other attribute values besides the expansion. either front or back could be used here
	int usableRowIndex = rule.rowForRuleFront[0];
	int appliedRows = 0;
	for (int row = 0; row < data.size(); row++)
	{
		bool goodRow = false;
		for (int col = 0; col < data.at(0).size(); col++)//There is no "-1" here because we want to account for the class... I think
		{
			auto it = std::find(epxandedColIndexes.begin(), epxandedColIndexes.end(), col);
			//if expanded attribute
			if (it != epxandedColIndexes.end())
			{
				int index = std::distance(epxandedColIndexes.begin(), it);
				//if the val of the current row and col is not within the expanded bounds of the rule, then go the next row
				if (!(data.at(row).at(col) >= minColVals[index] && data.at(row).at(col) <= maxColVals[index]))
				{
					goodRow = false;
					break;
				}
				goodRow = true;
			}
			else
			{
				//if the current row at the current column does not match the usable row at the current column, then the rule does not apply to that row
				//go to the next
				if (!(data.at(row).at(col) == data.at(usableRowIndex).at(col)))
				{
					goodRow = false;
					break;
				}
				goodRow = true;
			}
		}
		if (goodRow)
		{
			appliedRows++;
		}
	}
	return appliedRows;
}

//takes in a rule and returns the number of cases in the data which the rule covers
vector<Rule> testRules(vector<Rule> rules, vector<vector<int>> data)
{
	for (int i = 0; i < rules.size(); i++)
	{
		rules[i].coverageCounter = testRule(rules[i], data);
	}
	return rules;
}

//vector<vector<int>> removeDataFromBothClasses 

//removes duplicate rows with different classes, for example
//0, 1, 2, 1
//0, 1, 2, 2
//(considering that the class is in the last column)
vector<vector<int>> removeDups(vector<vector<int>> dat)
{
	// Initialize an unordered map to store the rows that have the same data
	unordered_map<string, int> mp;

	// Get the number of rows and columns in dat
	int n = dat.size(), m = dat[0].size();

	// Iterate through each row in dat
	for (int i = 0; i < n; i++)
	{
		string s = "";

		// Concatenate all elements in the row except for the last one (the class)
		for (int j = 0; j < m - 1; j++)
		{
			s += to_string(dat[i][j]) + " ";
		}

		// If a row is found that has the same data as another row but different classes then both rows are replaced with a row of -1s
		if (mp.find(s) != mp.end())
		{
			if (dat[i][m - 1] != dat[mp[s]][m - 1])
			{
				dat[i] = vector<int>(m, -1);
				dat[mp[s]] = vector<int>(m, -1);
			}
		}
		else
		{
			mp[s] = i;
		}
	}
	return dat;
}

// drive function
vector<string> doMichaelRuleGen(std::string filePath, vector<string> output, double coveragePercentage)
{
	/*vector<vector<double>> data;
	data = getArrayFromCSV("C:\\Users\\infer\\Desktop\\Github2\\Boris\\SPC-3D\\Assets\\FileData\\IrisBackUp.data");*/
	vector<vector<int>> data = readCSV(filePath);

	data = removeDups(data);

	//std::sort(data.begin(), data.end());

	//// Remove consecutive duplicates
	//auto last = std::unique(data.begin(), data.end());
	//data.erase(last, data.end());

	//// Create a set of vectors to store unique vectors
	//std::set<std::vector<int>> uniqueVectors(data.begin(), data.end());

	//// Clear the original vector and insert the unique vectors
	//data.clear();
	//data.insert(data.end(), uniqueVectors.begin(), uniqueVectors.end());

	// create vector of rules generated
	vector<Rule> rules;

	// create temporary rule structure for information tracking
	Rule* tempRule;

	//this holds all expansions and expansion data; check struct above for more detail
	unordered_map<int, Expansion> expansions;

	// compare the first case against all cases to find where the absolute value of all points subtracted is 1
	// Ex: c1 = {0, 0, 1, 2} and c2 = {1, 0, 1, 2}
	// c1 - c2 = {-1, 0, 0, 0}
	// sum the result = -1
	// take the absolute value = 1
	for (int compXIndex = 0; compXIndex < data.size(); compXIndex++)
	{
		// int vector to store the index of cases which have an absolute value of 1 when compared to the comparison case
		vector<int> matches;

		// vector to hold the difference between the two cases (see example below)
		vector<int> difference;

		// clear matches
		matches.clear();

		// push the current data being compared into the first index of the matches vector
		matches.push_back(compXIndex);

		// comparison case to keep track of current case to be compared
		vector<int> comparisonCase = data.at(compXIndex);

		for (int dataXIndex = 0; dataXIndex < data.size(); dataXIndex++)
		{
			for (int dataYIndex = 0; dataYIndex < (data.at(dataXIndex).size() - 1); dataYIndex++)
			{
				// check that the comparisonX and the dataX are not the same
				if (compXIndex != dataXIndex)
				{
					// subtract the comparison data from the data being compared
					difference.push_back(((comparisonCase.at(dataYIndex)) - (data.at(dataXIndex).at(dataYIndex))));
				}

			}// close dataY

			// add together the difference and check if the absolute value of the difference is 1
			// integer to hold the sum
			int sum = 0;
			for (int diff = 0; diff < difference.size(); diff++)
			{
				// cout << "Difference Value: " << difference.at(diff) << endl;
				sum += difference.at(diff);
			}

			// cout << "Difference Sum ABS: " << abs(sum) << endl;

			// clear difference vector
			difference.clear();

			// check if the absolute value of sum is 1 and if the classes are the same
			if (abs(sum) == 1 && data.at(compXIndex).at(data.at(compXIndex).size() - 1) == data.at(dataXIndex).at(data.at(dataXIndex).size() - 1))
			{
				//cout << "Match " << dataXIndex << " pushed to vector" << endl;
				matches.push_back(dataXIndex);
			}

		}// close dataX

		/*
		// print matches to ensure it is working
		for (int i = 0; i < matches.size(); i++)
		{
			if (i == 0)
			{
				cout << "ID" << matches.at(i) << " has the matches of: " << endl;
			}
			else
			{
				cout << "\tMatch " << i << ":" << " ID" << matches.at(i) << endl;
			}
		}
		*/

		// check if the matches can be expanded
		for (int match = 0; match < matches.size(); match++)
		{
			// boolean variable to check if an expandable attribute has already been found
			// this needs to be checked since there can only be one expansion
			bool expansionFound = false;

			// temporary vector to hold this comparisons expandable flags
			vector<int> tempExpansionDirection;

			// temporary vector to hold this comparisons expandable IDs
			vector<int> tempId;

			// temporary vector to hold the index of the expansion
			vector<int> tempColumnInd;

			// the first value in the match vector is the id of the original compared data
			if (match > 0)
			{
				// iterate over the matched dataID to determine if it can be expanded
				//changed dataIndex -> columnIndex
				for (int columnIndex = 0; columnIndex < (data.at(matches.at(match)).size() - 1); columnIndex++)
				{

					// check if the match can be expanded upwards
					if (expansionFound == false && comparisonCase.at(columnIndex) == (data.at(matches.at(match)).at(columnIndex) + 1))
					{
						// set the expansion found flag to true
						expansionFound = true;

						// set the expand flag to show it is an downward expansion
						//changed tempFlags -> tempExpansionFlags
						tempExpansionDirection.push_back(-1);

						// record the expansion index
						//change tempInd -> tempColumnInd
						tempColumnInd.push_back(columnIndex);

						/*
						cout << "Downward Expansion Found in match #" << match << endl;
						cout << "\tData at: " << dataIndex << " is: " << comparisonCase.at(dataIndex) << endl;
						cout << "\tExpansion: " << matches.at(match) << " at: " << match << " is: " << (data.at(matches.at(match)).at(dataIndex) + 1) << endl;
						*/
					}
					// check if the match can be expanded downwards
					else if (expansionFound == false && comparisonCase.at(columnIndex) == (data.at(matches.at(match)).at(columnIndex) - 1))
					{
						// set the expansion found flag to true
						expansionFound = true;

						// set the expand flag to show it is an upward expansion
						tempExpansionDirection.push_back(1);

						// record the expansion index
						tempColumnInd.push_back(columnIndex);

						/*
						cout << "Upward Expansion Found in match #" << match << endl;
						cout << "\tData at: " << dataIndex << " is: " << comparisonCase.at(dataIndex) << endl;
						cout << "\tExpansion: " << matches.at(match) << " at: " << match << " is: " << (data.at(matches.at(match)).at(dataIndex) + 1) << endl;
						*/
					}
					// this means that there are more than one expansions in the match
					// Ex: comparison Case = {0, 1, 0 ,0}
					// 	   match case = {1, 0, 0, 1}
					// In the above example, the difference in the sums is an absolute value of 1, but there is more than one possible expansion
					else if (expansionFound == true && comparisonCase.at(columnIndex) != (data.at(matches.at(match)).at(columnIndex)))
					{
						// remove the last flag pushed into the expansion flag vector
						if (tempExpansionDirection.size() > 0)
						{
							tempExpansionDirection.pop_back();
						}

						if (tempColumnInd.size() > 0)
						{
							tempColumnInd.pop_back();
						}

						break;
					}
					// check if there is an expansion that is too large or too small
					// Ex: ID1 (0, 0, 0, 1), ID2 (0, 0, 0, 3)
					// Ex: ID1 (0, 0, 0, 3), ID2 (0, 0, 0, 1)
					else if ((comparisonCase.at(columnIndex) > (data.at(matches.at(match)).at(columnIndex) + 1)) || (comparisonCase.at(columnIndex) < (data.at(matches.at(match)).at(columnIndex) - 1)))
					{
						// clear the last flag pushed if there are any
						if (tempExpansionDirection.size() > 0)
						{
							tempExpansionDirection.pop_back();
						}

						break;
					}
					// else the comparison case and the data are the same value
					else
					{
						continue;
					}
				}
				// end dataIndex

				if (expansionFound == true && tempExpansionDirection.size() > 0)
				{
					// add the comparison ID to the expandable vector
					tempId.push_back(matches.at(0));

					// add the match which can be expanded to to the expandable vector
					tempId.push_back(matches.at(match));

					Expansion exp;
					//this index uses rowDominant form. It is important to do this incase a row has expansions in different attributes/columns
					//thus there will be an expansion struct for each of those expansions
					//thus those expansions need different indexes
					int expansionsIndex = matches.at(0) + (tempColumnInd.at(0) * data.at(matches.at(0)).size() - 1);

					//the expansion index does not exist
					if (expansions.find(expansionsIndex) == expansions.end())
					{
						//create one
						expansions[expansionsIndex] = exp;
					}
					//fill the rowExpandedFrom and the columnIndex for the individual expansion
					expansions[expansionsIndex].rowExpandedfrom = matches.at(0);
					expansions[expansionsIndex].columnIndex = tempColumnInd.at(0);

					//if the expansionDirection of match is upwards, then the expansion and direction should 
					// go the front of "expansionIndexExpandedTo" and "expansionDirection" within expansions. 
					// Otherwise, both will go to the back. 
					//this is how the data is expected to be organized for later on down the road. 
					if (tempExpansionDirection.at(0) == 1)
					{
						expansions[expansionsIndex].expansionDirection.push_front(tempExpansionDirection.at(0));
						expansions[expansionsIndex].expansionIndexExpandedTo.push_front(matches.at(match) + (tempColumnInd.at(0) * data.at(matches.at(0)).size() - 1));
					}
					else
					{
						expansions[expansionsIndex].expansionDirection.push_back(tempExpansionDirection.at(0));
						expansions[expansionsIndex].expansionIndexExpandedTo.push_back(matches.at(match) + (tempColumnInd.at(0) * data.at(matches.at(0)).size() - 1));
					}
					//cout << expansions.size();
				}
			}
		} // end matches

		// clear the expandable and expansion flags
		// expandable.clear();
		// expansionFlags.clear();

	}// close compXIndex

	//cout << "\n";

	//// print the expansions to ensure it is working
	/*for (int i = 0; i < expandable.size(); i++)
	{
		for (int j = 0; j < expandable.at(i).size(); j++)
		{
			if (j == 0)
			{
				cout << "ID" << expandable.at(i).at(j) << " can expand to: " << endl;
			}
			else
			{
				cout << "\tExpand " << i << ":" << " ID" << expandable.at(i).at(j) << endl;
				cout << "\tExpand " << i << " Direction " << ": " << expansionFlags.at(i).at(j - 1) << " at index: " << columnExpansionIndexes.at(i).at(j - 1) << endl;
			}
		}
	}*/

	//cout << "\n";

	//////////////////=======================Here is where the values are differnt it seems; expansions===================//////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// check for "a <= X <= b" rules
	// Condition: index is the same
	//			  Ex: column index = 0
	//			  Data 1 = {0, 1, 1, 1}
	//			  Data 2 = {1, 1, 1, 1}
	//			  Data 3 = {2, 1, 1, 1}
	//			  Max for X1 = 2, min = 0
	//			  Rule: 0 <= x1 <= 2, x2 = 1, x3 = 1, x4 = 1

	//here's the gist the following loop:
	// Look at an expansion. Follow through all of it's expansions (in the same attribute) both up and down until there are not more expansions.
	// while doing so, mark each expansion as being in a rule, so that it is not used again--this eliminates non-encompassing rules. 
	// then make a rule based off of that. 
	// then move onto the next expansion. If that next expansion is already in a rule, then move on again. Keep doing that until you find an expansion not already in a rule
	// then start from step 1. 
	// 
	//loops through all expansions in the unordered_map filled in above
	for (auto it : expansions)
	{
		//it.first represents the key, and it.second represents the value--in this case the current expansion
		Expansion curExpansion = it.second;

		//expansionChain will hold the minimum and maximum expansions
		list<int> expansionChainRow;
		expansionChainRow.push_back(curExpansion.rowExpandedfrom);

		list<int> expansionIndexChain;
		expansionIndexChain.push_back(it.first);

		tempRule = new Rule;
		tempRule->colExpansionIndex.push_back(curExpansion.columnIndex);

		//to stop the while loop below
		bool endExpansionRuleCheck = false;

		//going to loop through all of the expansions that can be expanded from the intitial curExpansion
		while (endExpansionRuleCheck == false)
		{
			//if both bools are true, as seen in the bottom if statement, then the while loop stops and goes onto the next expansion. 
			bool noMoreUpExpansions = false;
			bool noMoreDownExpansions = false;

			//the index within expansions--uses rowDominantForm
			int currentExpansionIndexUp = expansionIndexChain.front();
			int currentExpansionIndexDown = expansionIndexChain.back();// +(curExpansion.columnIndex * data.at(expansionChain.back()).size() - 1);

			//the index within expansions of the next expansion--uses rowDominantForm
			int nextExpansionIndexUp = expansions[currentExpansionIndexUp].expansionIndexExpandedTo.front();
			int nextExpansionIndexDown = expansions[currentExpansionIndexDown].expansionIndexExpandedTo.back();

			//if the next expansion is not already in a rule and it's expansion direction from the rowExpandedFrom is uppward
			//then push it to the front of the expansion chain for the next loop
			if (expansions[currentExpansionIndexUp].expansionDirection.front() == 1 && expansions[nextExpansionIndexUp].alreadyInARule == false)
			{
				expansionChainRow.push_front(expansions[nextExpansionIndexUp].rowExpandedfrom);
				expansionIndexChain.push_front(nextExpansionIndexUp);
			}
			//otherwise mark that there are no more upward expansions from the intitial curExpansion
			else
			{
				noMoreUpExpansions = true;
			}

			//if the next expansion is not already in a rule and it's expansion direction from the rowExpandedFrom is downward
			//then push it to the back of the expansion chain for the next loop
			if (expansions[currentExpansionIndexDown].expansionDirection.back() == -1 && expansions[nextExpansionIndexDown].alreadyInARule == false)
			{
				expansionChainRow.push_back(expansions[nextExpansionIndexDown].rowExpandedfrom);
				expansionIndexChain.push_back(nextExpansionIndexDown);
			}
			//otherwise mark that there are no more downward expansions from the intitial curExpansion
			else
			{
				noMoreDownExpansions = true;
			}

			//mark that the used current up and down expansions are already in a rule so they are not used again
			//this eliminates non-encompassing rules
			//for example imagine the rules :
			//	R1: 1 <= X <= 2
			//	R2: 0 <= X <= 3
			//only R2 is needed since it fully covers R1. R1 is not encompasssing
			expansions[currentExpansionIndexUp].alreadyInARule = true;
			expansions[currentExpansionIndexDown].alreadyInARule = true;

			//if there are no more upward or downward expansions then break the while loop
			if (noMoreDownExpansions && noMoreUpExpansions)
			{
				endExpansionRuleCheck = true;
			}
		} // end while loop

		// determine if a rule has been generated
		if (expansionChainRow.size() > 1)
		{
			// add the data to the temporary rule so that it can be added to the rule vector
			// copy the coordinates into the ruleVals vector
			int max = data.at(expansionChainRow.front()).at(tempRule->colExpansionIndex[0]);
			int min = data.at(expansionChainRow.back()).at(tempRule->colExpansionIndex[0]);
			if (min > max)//unfortunate brute force method where sometimes the min and max are reversed so they have to be fixed
			{
				int temp = expansionChainRow.back();
				expansionChainRow.back() = expansionChainRow.front();
				expansionChainRow.front() = temp;
			}
			else if (min == max)
			{
				goto ENDFORLOOP;
			}
			tempRule->rowForRuleFront.push_back(expansionChainRow.front());
			tempRule->rowForRuleBack.push_back(expansionChainRow.back());

			// calculate coverage of the rule
			tempRule->coverageCounter = expansionChainRow.size();

			// put the class in the rule
			tempRule->classCovered = data.at(tempRule->rowForRuleFront[0]).at(data.at(tempRule->rowForRuleFront[0]).size() - 1);

			// put the tempRule into the rule vector
			rules.push_back(*tempRule);

			// clear the tempRule so it can be used again
			clearRule(*tempRule);

			// clear the expansionChain so it can be used again

		}
	ENDFORLOOP:
		expansionChainRow.clear();
		expansionIndexChain.clear();
	} // end expansion outer for loop


	//counting number of cases per class. Currently only recognizes two classes
	int class1Num = 0;
	int class2Num = 0;
	//determine the column index in the data which holds the class number (the last column in the data)
	int columnCount = data.at(0).size() - 1;
	//checks the last class column for every set (row of data) and increases either class1Num or class2Num depending on if the set is in class 1 or 2
	for (int i = 0; i < data.size(); i++)
	{
		if (data.at(i).at(columnCount) == 1)
		{
			class1Num++;
		}
		else
		{
			class2Num++;
		}
	}

	//cout << "Total rules: " << rules.size() << endl;
	rules = testRules(rules, data);
	std::sort(rules.begin(), rules.end(), compareByCoverage);
	rules = combineOverlappingRules(rules, data);
	rules = testRules(rules, data);
	std::sort(rules.begin(), rules.end(), compareByCoverage);
	rules = combineRulesGoodClauses(rules, data);
	rules = testRules(rules, data);
	std::sort(rules.begin(), rules.end(), compareByCoverage);


	// print results
	int ruleCount = 0;


	for (int i = 0; i < rules.size(); i++)
	{
		int curClass = rules.at(i).classCovered == 1 ? class1Num : class2Num;
		double classCoveragePercent = ((double)rules.at(i).coverageCounter / curClass * 100);
		double allCaseCoveragePercent = ((double)rules.at(i).coverageCounter / data.size() * 100);

		/*if (classCoveragePercent < coveragePercentage)
		{
			continue;
		}*/

		ruleCount++;
	}

	output.push_back("Total rules: " + to_string(ruleCount) + "\n");

	ruleCount = 1;

	for (int i = 0; i < rules.size(); i++)
	{

		int curClass = rules.at(i).classCovered == 1 ? class1Num : class2Num;
		double classCoveragePercent = ((double)rules.at(i).coverageCounter / curClass * 100);
		double allCaseCoveragePercent = ((double)rules.at(i).coverageCounter / data.size() * 100);

		if (classCoveragePercent < coveragePercentage)
		{
			continue;
		}

		ruleCount++;

		for (int dataPrint = 0; dataPrint < data.at(0).size() - 1; dataPrint++)
		{
			int front = data.at(rules.at(i).rowForRuleFront[0]).at(dataPrint);

			auto it = std::find(rules.at(i).colExpansionIndex.begin(), rules.at(i).colExpansionIndex.end(), dataPrint);

			if (it != rules.at(i).colExpansionIndex.end())
			{
				int index = std::distance(rules.at(i).colExpansionIndex.begin(), it);

				int back = data.at(rules.at(i).rowForRuleBack[index]).at(dataPrint);
				front = data.at(rules.at(i).rowForRuleFront[index]).at(dataPrint);

				output.push_back(to_string(back) + " <= X" + to_string(dataPrint + 1) + " <= " + to_string(front) + "\n");
			}
			else
			{
				output.push_back( "X" + to_string(dataPrint + 1) + " = " + to_string(front) + "\n");
			}

		}

		// print the class the rule covers
		output.push_back("covers class " + to_string(rules.at(i).classCovered) + "\n");

		output.push_back("Number of cases covered: " + to_string(rules.at(i).coverageCounter) + "\n");
		
		output.push_back("Coverage of class: " + to_string(classCoveragePercent) + "%\n");

		output.push_back("Coverage of all cases: " + to_string(allCaseCoveragePercent) + "%\n");
		// cout << "Number of class 1 cases covered: " << rules.at(i).classCovered << endl;

		output.push_back("\n\n");

		ruleCount++;
	}

	return output;
} // end of main method

