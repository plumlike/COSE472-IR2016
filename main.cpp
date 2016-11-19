#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <functional>
#include <map>
#include <stdio.h>
#include "porter2_stemmer.h"
#include "time.h"
#include "utility.h"
#include <queue>

using namespace std;

struct news {
	string docno;
	vector<string> headline;
	vector<string> text;
};
// 단어 정보를 저장할 구조체
struct wordinfo {
	int id;
	int cf;
	int df;
	int start; // 역색인 파일에서 단어의 시작 행 번호
	//unordered_map<int, int> tf; // 문서id, 단어가 해당 문서에서 등장한 횟수
	wordinfo() : cf(1), df(0) {}
};

// 단어와 단어 정보를 매핑
unordered_map<string, wordinfo> word_data;

map<string, wordinfo> wordData;

vector<string> tokenizer(string s, char delim) {
	vector<string> tokens;
	// vector<string> tokens;
	stringstream ss(s); // 문자열을 가지고 스트림을 만든 다음 getline 함수로 띄어쓰기 단위로 파싱하여 벡터에 넣을꺼
						// 아깐 파일스트림을 getline으로 한줄씩 읽은것이므로 동일한 형식입니당
	string token;
	while (getline(ss, token, delim)) { // getline의 세 번째 인자에 유의할 것! 띄어어쓰기단위로...
		tokens.push_back(token);
	}
	return tokens;
}

// tokenize 된 headline 또는 text를 불용어 제거, 스테밍 한 뒤 결과 문자열을 반환
// news.headline = stemming(tokenize(news.headline), stopwords);
vector<string> stemming(vector<string> str, unordered_set<string>& stopwords) {
	vector<string> result;
	for (int i = 0; i < str.size(); ++i) {
		Porter2Stemmer::trim(str[i]);
		if (str[i] != "" && stopwords.find(str[i]) == stopwords.end()) {
			Porter2Stemmer::stem(str[i]);
			if (str[i] != "" && stopwords.find(str[i]) == stopwords.end()) {
				result.push_back(str[i]);
			}
		}
	}
	return result;
}

//void printTokens(vector<string>& tokens) {
//	for (int i = 0; i < tokens.size(); ++i) {
//		if ((tokens[i] == "[DOCNO] : ") || (tokens[i] == "[HEADLINE] : ") || (tokens[i] == "[TEXT] : "))
//			cout << endl;
//		cout << tokens[i] << " ";
//	}
//}

bool is_valid_char(char c) {
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '<' || c == '>'
		|| c >= '0' && c <= '9' || c == '.' || c == '/') return true;
	return false;
}

void tokenize_only_alpha(string s, vector<string>& tokens) {
	string tmp;
	bool is_counting = false;
	tokens.clear();
	for (int i = 0; i < s.size(); ++i) {
		if (!is_counting && is_valid_char(s.at(i))) {
			is_counting = true;
			tmp = s.at(i);
		}
		else if (is_counting && is_valid_char(s.at(i))) {
			tmp += s.at(i);
		}
		else if (is_counting && !is_valid_char(s.at(i))) {
			is_counting = false;
			tokens.push_back(tmp);
		}
	}
	if (is_counting) {
		tokens.push_back(tmp);
	}
}

news parse(ifstream &file) {
	string line;
	string tmp;
	news doc;
	int idx, idx2;
	do {
		getline(file, line);
		if ((idx = line.find("<DOCNO>")) != string::npos) {
			tmp = "";
			if ((idx2 = line.find("</DOCNO>")) != string::npos) {
				doc.docno = line.substr(idx + 8, 16);
			}
			else {
				getline(file, line);
				do {
					tmp += line + " ";
					getline(file, line);
				} while (line.find("</DOCNO>") == string::npos);
				doc.docno = tmp;
			}
		}
		if ((idx = line.find("<HEADLINE>")) != string::npos) {
			tmp = "";
			if ((idx2 = line.find("</HEADLINE>")) != string::npos) {
				tokenize_only_alpha(line.substr(idx + 10, line.size() - 10 - 11), doc.headline);
			}
			else {
				getline(file, line);
				do {
					tmp += line + " ";
					getline(file, line);
				} while (line.find("</HEADLINE>") == string::npos);
				tokenize_only_alpha(tmp, doc.headline);
			}
		}
		if ((idx = line.find("<TEXT>")) != string::npos) {
			tmp = "";
			if ((idx2 = line.find("</TEXT>")) != string::npos) {
				tokenize_only_alpha(line.substr(idx + 6, line.size() - 6 - 7), doc.text);
			}
			else {
				getline(file, line);
				do {
					tmp += line + " ";
					getline(file, line);
				} while (line.find("</TEXT>") == string::npos);
				tokenize_only_alpha(tmp, doc.text);
			}
		}
	} while (line.find("</DOC>") == string::npos);
	return doc;
}

int word_id = 0;
int doc_id = 0;
int total_doc_num;

void update_word_info(string word, map<string, int>& tempTF) {
	// 단어와 단어 정보 매핑.
	if (word_data.find(word) == word_data.end()) {
		// 처음 등장하는 단어이므로 id부여. cf의 초기값은 1이된다. (전체에서 처음 등장)
		word_data[word] = wordinfo();
	}
	else {
		word_data[word].cf++;
	}
	// 문서의 단어들에 대해 TF 구함.
	if (tempTF.find(word) == tempTF.end()) {
		// TF에 없다는 의미는 현재 문서에서 처음 등장한 단어라는 의미.
		tempTF[word] = 1;
		word_data[word].df++;
	}
	else {
		tempTF[word]++;
	}
}

string format_digit(int num_digit, int num) { // 색인어ID,문서ID,TF를 원하는 자릿수로 표현
	string result = "";
	for (int i = 0; i < num_digit; ++i) {
		result = result + (to_string(num % 10));
		num /= 10;
	}
	reverse(result.begin(), result.end());
	return result;
}

string format_weight(double weight) {
	string results = "";
	ostringstream os;
	os << weight;

	string str = os.str();

	if (weight == (int)weight) {
		str.append(".0");
	}

	if (str.size() > 7) {
		str = str.substr(0, 7);
	}
	else {
		while (str.size()<7) {
			str.append("0");
		}
	}
	return str;
}

void transformDocument(news& doc, unordered_set<string> &stopwords, ofstream& outfile, ofstream& doc_data_file) {
	doc_id++; // 문서 아이디. DOC 하나당 하나씩 증가
	vector<string> v1 = stemming(doc.headline, stopwords);
	vector<string> v2 = stemming(doc.text, stopwords);

	// 위에서 스테밍 된 하나의 문서에 대해서 그 문서의 단어들에 대한 TF값을 구한다.
	map<string, int> tempTF;
	for (auto word : v1) {
		update_word_info(word, tempTF);
	}
	for (auto word : v2) {
		update_word_info(word, tempTF);
	}

	// tempTF 에 현재 문서에서 나오는 단어들의 TF정보가 있으므로 TF.txt 파일을 생성할 수 있다.
	for (auto word : tempTF) {
		outfile << doc_id << '\t' << word.first << '\t' << word.second << endl;
	}

	//에 행 추가.
	doc_data_file << format_digit(6,doc_id) << doc.docno << format_digit(4,v1.size() + v2.size()) << endl;

	// Clear
	doc.headline.clear();
	doc.text.clear();
}

void findRelevant(ifstream& indexIf, unordered_map<string, int> &query, unordered_map<string, unordered_map<int, int>>& tf_memory, set<int>& relevant) {
	map<int, double> tempRelevant; // 문서id, 문서 score
	int df, tf, docId;
	double weight;// 이 문서에서의 단어의 가중치를 담음 (이 단어의 문서에서의 가치)
	unordered_map<int, int> tf_mapping;
	string tmp, line;

	for (auto& q : query) //일단 한번이라도 쿼리단어가 등장한 document를 골라놓는다.
	{
		indexIf.seekg((unsigned long)(6 + 6 + 7 + 3 + 2) * wordData[q.first].start, indexIf.beg);
		df = wordData[q.first].df;
		if (df > 15000) df = 4000;
		for (int i = 0; i < df; ++i) {
			getline(indexIf, line);
			memcpy(&tmp[0], &line[0] + 6, 6);
			tmp[6] = 0;
			//tmp = line.substr(6, 6);
			docId = stoi(tmp);

			memcpy(&tmp[0], &line[0] + 12, 3);
			tmp[3] = 0;
			tf = stoi(tmp);
			//tf = stoi(line.substr(12, 3));

			memcpy(&tmp[0], &line[0] + 15, 7);
			tmp[7] = 0;
			weight = stod(tmp);
			//weight = stod(line.substr(15, 7));
			if (tf_memory.find(q.first) == tf_memory.end()) {
				tf_memory[q.first] = tf_mapping;
			}
			tf_memory[q.first][docId] = tf;
			if (tempRelevant.find(docId) == tempRelevant.end()) {
				tempRelevant[docId] = (double)q.second * q.second / query.size() * weight;
			}
			else {
				tempRelevant[docId] += (double)q.second * q.second / query.size() * weight;
			}
		}
	}
	int count = 0;

	//1번만 등장한 문서들중에서, 단어가 많이 등장한 문서순서로 랭킹을 하기위한 작업
	vector<pair<double, int>> forSort;
	for (auto& rank : tempRelevant) {
		forSort.push_back(make_pair(rank.second, rank.first));
	}
	sort(forSort.begin(), forSort.end(),greater<pair<double,int>>()); //내림차순으로 문서랭킹

	for (auto& p : forSort) { // 선정된 document중 원하는기준(relevantNum)을 만족하는 단어를 찾는다.
		relevant.insert(p.second);
		//cout << p.second << " " << p.first << " " << endl;
		count++;
		if (count == 1000) break;
	}
}

int get_doc_length(int doc_id) {
	ifstream doc("doc.dat");
	string line;
	doc.seekg((unsigned long long)(doc_id - 1)*(6 + 16 + 4 + 2), doc.beg);
	getline(doc, line);
	return stoi(line.substr(6+16, 4));
}

void languageModel(set<int>& relevantSet, unordered_map<string, int>& query,
										int C, unordered_map<string, unordered_map<int,int>> tf_mem, vector<pair<double, int>>& result_set)
{
	int tf, df;
	map<int, int> doc_len;		// 문서번호, 문서길이
	map<int, double> dirichlet;	// 문서번호, 문서 score
	double mu = 3500;
	for (auto& doc_id : relevantSet) {
		doc_len[doc_id] = get_doc_length(doc_id);
		for (auto& word : query) {
			if (tf_mem[word.first].find(doc_id) != tf_mem[word.first].end()) {
				// 문서에 쿼리 단어가 있을 때
				dirichlet[doc_id] += log((tf_mem[word.first][doc_id] + mu*wordData[word.first].cf / C) / (doc_len[doc_id] + mu));
			}
			else {
				// 문서에 쿼리 단어가 없을 때 (smoothing)
				dirichlet[doc_id] += log((mu*wordData[word.first].cf / C) / (doc_len[doc_id] + mu));
			}
		}
	}

	for (auto& res : dirichlet) {
		result_set.push_back(make_pair(res.second, res.first));
	}
	sort(result_set.begin(), result_set.end(), greater<pair<double,int>>());
}

//문서의 유사도점수, 문서id를 vectorSpaceModel에 따라 리턴
vector<pair<double, int>> vectorSpaceModel(set<int> &relevants, unordered_map<string,int> &query) {
	map<int, pair<double, double>> cosine; //문서id, 분자, 분모
	ifstream indexFile("index.dat");
	string line;
	int id;
	double weight;

	for (auto &q : query) {
		if (wordData.find(q.first) == wordData.end()) continue;
		unsigned long long start = wordData[q.first].start;
		if (wordData.find(q.first) != wordData.end()) {
			indexFile.seekg((6 + 6 + 3 + 7 + 2) * start, indexFile.beg);
			for (int i = 0; i < wordData[q.first].df; i++) {
				getline(indexFile, line);

				weight = stod(line.substr(6 + 6 + 3, 7)); //index.dat가 단어id 문서id tf 가중치

				id = stoi(line.substr(6, 6)); // 문서 id
				
				if (relevants.find(id) == relevants.end()) continue;

				if (cosine.find(id) == cosine.end()) {

					cosine.insert(make_pair(id, make_pair(weight*q.second, weight*weight)));
				}
				else {
					cosine[id].first += weight*q.second;
					cosine[id].second += weight*weight;
					/*cout << cosine[id].first << endl;   
					cout << cosine[id].second << endl;*/
				}
			}
		}
	}
	indexFile.close();
	vector<pair<double, int>> result; // pair<double, int> = pair<코사인 similarity 계산된값, 문서id>
	for (auto tmp : cosine) {
		result.push_back(make_pair(tmp.second.first / sqrt(tmp.second.second), tmp.first));
	}
	return result;
}

string get_doc_name_by_id(int doc_id) {
	ifstream doc("doc.dat");
	string line;
	doc.seekg((unsigned long long)(6 + 16 + 4 + 2)*(doc_id - 1), doc.beg);
	getline(doc, line);
	doc.close();
	return line.substr(6, 16);
}

vector<pair<int, string>> inverted_index;

void prepare_index_file(ofstream& outfile, ifstream& infile) {
	string line;
	vector<string> tokens;
	map<string, int> word_tf;	// 단어와 단어의 TF 매핑. (임시)
	int prev = 1;
	int curr;
	double denom = 0.0f;
	while (getline(infile, line)) {
		tokens = tokenizer(line, '\t'); // 문서번호 TAB 단어 TAB TF
		curr = stoi(tokens[0]);
		if (prev != curr) { // 해당 문서의 단어들을 다 읽었음.
			// 분모 값 완성.
			for (auto word : word_tf) {
				inverted_index.push_back(make_pair(word_data[word.first].id,
					format_digit(6, prev) + format_digit(3, word.second) + format_weight((log(word.second) + 1) * 
						log((double)total_doc_num / word_data[tokens[1]].df) / sqrt(denom))));
			}
			// Initialization
			prev = curr;
			denom = 0.0f;
			word_tf.clear();

			// process current line (new document)
			word_tf[tokens[1]] = stoi(tokens[2]);
		}
		else {
			denom = denom + pow(log(stoi(tokens[2]) + 1) * log((double)total_doc_num / word_data[tokens[1]].df), 2);
			word_tf[tokens[1]] = stoi(tokens[2]);
		}
	}
	for (auto word : word_tf) {
		inverted_index.push_back(make_pair(word_data[word.first].id, format_digit(6, prev) + 
			format_digit(3, word.second) + format_weight((log(word.second) + 1) * log((double)total_doc_num / word_data[word.first].df) / sqrt(denom))));
	}
}

void write_index_file(ofstream& out) {
	sort(inverted_index.begin(), inverted_index.end());
	for (auto word : inverted_index) {
		out << format_digit(6, word.first) << word.second << endl;
	}
}

void printResult(int qNum, vector<pair<double, int>> &model, ofstream &file) {
	file << qNum << endl;
	for (auto result : model) {
		file << get_doc_name_by_id(result.second) << "\t";
	}
	file << endl;
}

int main()
{
	clock_t start,finish;
	// ctrl+shift+c 주석 ctrl+shift+x 해제
	string word;
	ifstream file;
	ifstream inFile;
	ofstream outfile;
	ofstream outfile2;
	vector<string> paths; // 인풋 파일 경로 집합

	string line;

	utility::get_file_paths(TEXT(".\\ir_corpus"), paths);

	//vector<string> tokens;

	// Preparation
	ifstream sw("stopword.txt");
	unordered_set<string> stopwords;
	while (getline(sw, line)) {
		stopwords.insert(line);
	}  
	Porter2Stemmer::irregular_word("irregular_word.txt");
	// END OF PREP

	start = clock(); // START CLOCK
	cout << "start ... " << endl;

	inFile.open("doc.dat");
	if(!inFile.is_open()) {
		outfile.open("tf.txt");
		outfile2.open("doc.dat");
		for (int i = 0; i < paths.size(); ++i) {
			file.open(paths[i]); 
			if (file.is_open()) {
				// Line by Line 으로 읽는 함수 : getline
				while (getline(file, line)) {
					if (line.find("<DOC>") != string::npos) {
						//tf.txt와 doc.dat 생성하러감.
						transformDocument(parse(file), stopwords, outfile, outfile2); // outfile=tf.txt, outfile2-doc.dat
					}
				}
				file.close();
			}
			else {
				cout << "File Not Found!" << endl;
			}
		}
		outfile.close();
		outfile2.close();
	}
	else inFile.close();

	// update total document numbers when transforming is done.
	total_doc_num = doc_id;

	// 단어정보파일 출력 (word_data 를 순회하면서 출력하면 된다.)
	inFile.open("term.dat");
	if (!inFile.is_open()) {
		outfile2.open("term.dat");
		int start_pos = 0;
		for (auto word : word_data) {
			word_id++;
			word_data[word.first].id = word_id;
			outfile2 << word_data[word.first].id << '\t' << word.first << '\t' << word_data[word.first].df << '\t'
				<< word.second.cf << '\t' << start_pos << endl;
			start_pos += word_data[word.first].df; // 다음 단어의 시작위치 = 현재 단어의 시작위치 + 현재 단어의 DF
		}outfile2.close();
	}
	else inFile.close();
	
	inFile.open("index.dat");
	if (!inFile.is_open()) {
		outfile2.open("index.dat");
		file.open("tf.txt");
		prepare_index_file(outfile2, file);
		write_index_file(outfile2);
		outfile2.close();
	}
	else inFile.close();

	// index.dat 쓰기

	//declair variables
	ifstream topics25("topics25.txt"); //ifstream은 읽을 때.. 
	string token, topicString;
	unordered_map<int, unordered_map<string, int>> query; //num, 단어, 단어의tf
	vector<string> tokens;
	int queryNum;

	//0. Term.dat 로딩
	ifstream wordFile;
	wordFile.open("term.dat");


	//존재하는 term.dat 읽어오기
	vector<string> docVec;
	vector<string> wordTokens;

	while (getline(wordFile, line)) {
		wordTokens.clear();
		wordTokens = tokenizer(line, '\t');
		wordData[wordTokens[1]] = wordinfo();
		wordData[wordTokens[1]].id = stoi(wordTokens[0]);
		wordData[wordTokens[1]].df = stoi(wordTokens[2]);
		wordData[wordTokens[1]].cf = stoi(wordTokens[3]);
		wordData[wordTokens[1]].start = stoi(wordTokens[4]);
	}
	wordFile.close();

	//1. 쿼리 정련
	while (getline(topics25, line)) {
		stringstream topicsStream(line);
		tokens.clear();
		while (getline(topicsStream, token, ' ')) { tokens.push_back(token); }
		if (tokens.size() == 0) continue;
		if (tokens[0] == "<top>" || tokens[0] == "</top>") { continue; }
		else if (tokens[0] == "<num>") {
			queryNum = stoi(tokens[2]);
			unordered_map<string, int> mapmap;
			query.insert(make_pair(queryNum, mapmap));
		}
		else if (tokens[0] == "<desc>") { continue; }
		else if (tokens[0] == "<narr>") { continue; }
		else if (tokens[0] == "<title>") {
			for (int i = 1; i < tokens.size(); ++i) {
				Porter2Stemmer::trim(tokens[i]);
				if (stopwords.find(tokens[i]) != stopwords.end()) continue;
				Porter2Stemmer::stem(tokens[i]);
				if (wordData.find(tokens[i]) == wordData.end()) continue;
				if (stopwords.find(tokens[i]) == stopwords.end() && tokens[i] != "") {
					if (query[queryNum].find(tokens[i]) != query[queryNum].end()) {
						(query[queryNum])[tokens[i]] += 5;//*= 2;
					}
					else {
						query[queryNum][tokens[i]] = 1;
					}
				}
			}
		}
		else {
			for (auto& word : tokens) {
				Porter2Stemmer::trim(word);
				if (stopwords.find(word) != stopwords.end()) continue;
				Porter2Stemmer::stem(word);
				if (wordData.find(word) == wordData.end()) continue;
				if (stopwords.find(word) == stopwords.end() && word != "") {
					if (query[queryNum].find(word) != query[queryNum].end()) {
						(query[queryNum])[word] += 3;//+= 5;
					}
					else {
						query[queryNum][word] = 1;
					}
				}
			}
		}
	} //쿼리 정련 끝.

	/*for (auto& q : query) {
		cout << q.first << endl;
		for (auto& word : q.second) {
			cout << word.first << " " << word.second << endl;
		}
		cout << endl;
	}*/
	

	// 전체 단어 개수 구하기 (언어모델에서 사용)
	int C = 0;
	for (auto &w : wordData) {
		C += w.second.cf;
	}

	ofstream resultTxt("result.txt");
	
	//ifstream indexIf("index.dat");
	////벡터스페이스.txt에 출력
	//for(auto &p : query){
	//	unordered_map<string, unordered_map<int, int>> tf_memory;
	//	set<int> relevants;
	//	vector<pair<double, int>> vsResult; 
	//	findRelevant(indexIf, p.second, tf_memory, relevants);
	//	//findRelevant(p.second, p.second.size() * 1 / 15,); // 쿼리와 relevant한 문서 집합 가져오기
	//	
	//	vectorSpaceModel(relevants, p.second);//vector space model로 문서점수,문서id 반환
	//	sort(vsResult.begin(), vsResult.end(),greater<pair<double,int>>()); // 벡터스페이스모델을 통해 구한 점수값 순으로 문서 정렬
	//	printResult(p.first,vsResult,resultTxt);
	//	vsResult.clear();
	//}	cout << " 벡터스페이스 완료 " << endl;
	
	clock_t totalt=0.0,st, et;
	////언어모델.txt출력	
	ifstream indexIf("index.dat");
	for (auto &p : query) {
		unordered_map<string, unordered_map<int,int>> tf_memory;			
		set<int> relevants;
		vector<pair<double, int>> vsResult;
	//	st = clock();
		findRelevant(indexIf, p.second, tf_memory, relevants); //쿼리의 relevant			
	//	et = clock();cout << "시간 : " << difftime(et, st) / 1000 << "초" << endl;
	//	totalt += difftime(et, st);
		languageModel(relevants, p.second, C, tf_memory, vsResult);//language model로 문서점수,문서id 반환	
		printResult(p.first, vsResult, resultTxt);
		
		vsResult.clear();

	}//cout << "total 시간 : " << totalt << " ms" << endl;
	resultTxt.close();
	cout << " 언어모델 완료 " << endl;
	indexIf.close();
	finish = clock();
	cout << "\n소요시간(s) : " << difftime(finish, start) / 1000 << "초"<<endl;
	system("pause");
	return 0;
}