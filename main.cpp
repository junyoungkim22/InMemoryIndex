#include <stdio.h>
#include "timer.hpp"
#include "util.hpp"
#include <assert.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#define INSERT 0
#define READ 1
#define UPDATE 2
#define SCAN 3

#define segSize 4 // bit size
#define childNum 16 // 2^(segSize)
#define maxLen 16 // 64/segSize

class Trie_node{
	public: 
		Trie_node* parent;
		Trie_node* child[childNum];
		bool isEnd;
		int8_t value;

		Trie_node(){
			parent = NULL;

			for(int i=0; i<childNum; i++)
				child[i] = NULL;

			isEnd = false;
		}
		
		~Trie_node(){
			for(int i=0; i<childNum; i++)
				if(child[i] != NULL)
					delete child[i];
		}

		void insert(uint64_t key, int8_t pValue, int8_t level){
//			printf("key: %lx, level: %d\n", (long) key, (int)level);
			if(level == maxLen){
				isEnd = true;
				value = pValue;
				return;
			}
			//int8_t localKey = (uint8_t)(&key)[level];
			//uint8_t localKey = (key >> (8*level)) & 0xff;
			uint8_t localKey = (key >> (segSize * level)) & 0x0f;
//			printf("localKey: %x\n", (unsigned) localKey);

			Trie_node* curNode = child[localKey];
			
			if(curNode ==  NULL){
				curNode = new Trie_node;
				curNode->parent = this;
				child[localKey] = curNode;
			}

			curNode->insert(key, pValue, level+1);

			return;
		}

		int8_t read(uint64_t key, int8_t level){
//			printf("read key: %lx\n", (long)key);
			if(level == maxLen)
				return value;
//			int8_t localKey = (int8_t)(&key)[level];
			uint8_t localKey = (key >> (segSize * level)) & 0x0f;
//			printf("localKey: %x\n", (unsigned) localKey);
			Trie_node* curNode = child[localKey];
//			printf("isNULL child[localKey]? %lx\n", curNode);
			return curNode->read(key, level+1);

/*			uint8_t localKey;
			Trie_node* curNode = this;
			for(int8_t level=0; level < maxLen; level++){
				localKey = (key >> (segSize * level)) & 0x0f;
				curNode = curNode->child[localKey];
			}
			return curNode->value;
*/		}
		
		void update(uint64_t key, int8_t pValue, int8_t level){
			//printf("update\n");
			if(level == maxLen){
				value = pValue;
				return;
			}
			
			uint8_t localKey = (key >> (segSize * level)) & 0x0f;
			//int8_t localKey = (int8_t)(&key)[level];
			Trie_node* curNode = child[localKey];
			curNode->update(key, pValue, level+1);
			return;
		}

		void scan(uint64_t key, uint8_t level, int8_t& num, uint64_t& result){
			if(level == maxLen){
				result += (uint64_t)value;
				num--;
				return;
			}

			//int8_t localKey = (int8_t)(&key)[level];
			uint8_t localKey = (key >> (segSize * level)) & 0x0f;
			for(int i=localKey; i<childNum; i++){
				Trie_node* curNode = child[localKey];
				
				if(curNode == NULL)
					continue;

				curNode->scan(key, level+1, num, result);
				if(num==0)
					break;
			}

			return;
		}
};

class InMemoryIndex {
public:
	Trie_node root;

	/*	Trie_node *root;

	InMemoryIndex(){
		root = new Trie_node;
	}
	
	~InMemoryIndex(){
		delete root;
	}
*/
	void insert(uint64_t key, int8_t value) {
/*		Trie_node* curNode = root;
		Trie_node* nxtNode = NULL;

		uint8_t localKey;

		for(int8_t level = 0; level < maxLen; level++){
			localKey = (key >> (segSize * level)) & 0x0f;
			nxtNode = curNode->child[localKey];

			if(nxtNode == NULL){
				nxtNode = new Trie_node;
				nxtNode->parent = curNode;
			}
		}
		*/
		root.insert(key, value, 0);
	}

	void update(uint64_t key, int8_t value) {
		root.update(key, value, 0);
	}

	int8_t read(uint64_t key) {
		return root.read(key, 0);
	}

	uint64_t scan(uint64_t key, int8_t num) {
		uint64_t result = 0;
		root.scan(key, 0, num, result);
		return result;
	}
};

struct Operation {
	int type;
	uint64_t key;
	int num;

	Operation() {
		type = -1; key = 0; num = 0;
	}
	Operation(int t, uint64_t k, int n) {
		type = t; key = k; num = n;
	}
};

InMemoryIndex tree;
std::vector<Operation> inits;
std::vector<Operation> txns;

double load (std::string &fname, std::vector<Operation> &ops, uint64_t limit) {
	Timer timer;
	timer.start();
	ops.reserve(limit);
	std::ifstream infile(fname);
	std::string op;
	uint64_t key;
	int num;
	for (uint64_t count = 0; count < limit && infile.good(); ++count) {
		infile >> op >> key;
		if (op.compare("INSERT") == 0) {
			ops.emplace_back(INSERT, key, 0); 
		} else if (op.compare("READ") == 0) {
			ops.emplace_back(READ, key, 0); 
		} else if (op.compare("UPDATE") == 0) {
			ops.emplace_back(UPDATE, key, 0); 
		} else if (op.compare("SCAN") == 0) {
			infile >> num;
			ops.emplace_back(SCAN, key, num); 
		}
	}
	infile.close();
	return timer.get_elapsed_time();
}

double exec_loads (std::vector<Operation> &ops) {
	Timer timer;
	timer.start();
	long int i=0;
	for (auto op = ops.begin(); op != ops.end(); ++op) {
		if(i%1000000 == 0) printf("%liM Load done.\n", i/1000000);
		i++;
		if (op->type == INSERT) {
			tree.insert(op->key, 1);
		} 
	}
	return timer.get_elapsed_time();
}

uint64_t read_result = 0;
uint64_t scan_result = 0;

double exec_txns (std::vector<Operation> &ops) {
	Timer timer;
	timer.start();
	long int i=0;
	for (auto op = ops.begin(); op != ops.end(); ++op) {
		if(i%1000000 == 0) printf("%liM Txns done.\n", i/1000000);
		i++;
		if (op->type == INSERT) {
			tree.insert(op->key, 1);
		} else if (op->type == READ) {
			read_result += tree.read(op->key);
		} else if (op->type == UPDATE) {
			tree.update(op->key, 2);
		} else if (op->type == SCAN) {
			scan_result += tree.scan(op->key, op->num);
		}
	}
	return timer.get_elapsed_time();
}

int main(int argc, char* argv[]) {
	std::string load_fname(argv[1]);
	std::string txns_fname(argv[2]);
	size_t load_size = stoi(argv[3]);
	size_t txns_size = stoi(argv[4]);

	std::cout << "load_fname: " << load_fname << "\n";
	std::cout << "txns_fname: " << txns_fname << "\n";

	load(load_fname, inits, load_size);
	std::cout << "Execute " << load_size << " operations\n";
	double load_time = exec_loads(inits);
	std::cout << "load time(ms): " << load_time << "\n";
	inits.clear();

	load(txns_fname, txns, txns_size);
	std::cout << "Execute " << txns_size << " operations\n";
	double txns_time = exec_txns(txns);
	std::cout << "txns time(ms): " << txns_time << "\n";
	std::cout << "sum of values of read entries: " << read_result << "\n";
	std::cout << "sum of values of scanned entries: " << scan_result << "\n";
	return 1;
}
