/*
Assumption and design choices:
- get_last and get_first returns key-value pairs that exist on the hash table. Removed pairs don't count.
- If the load factor (number of elements (n) / size of hash table) goes over a threshold, we should resize the table by a factor of two
and insert all elements in the table again. This would run in O(n). As it's not directly stated in the problem, we assume it'll be big enough for this case.
- The file is read locally.
- In case of collision, we look into i+1, i+2,... until finding an empty slot
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

// Status of each slot in the hash table
enum Status{
    EMPTY, // Slot never filled
    FILLED, // Slot holds a key-value pair
    REMOVED // Slot had its key-value removed
};

// Each slot of the hash table contains a "node". Each node contains a key-value pair, and references to the previous and following inserted pairs (double linked list). 
// This allows for O(1) operations in get_last and get_first
class Node{
    private:
        Node* next = NULL; // Points to next inserted pair
        Node* prev = NULL; // Points to previous inserted pair
        string key; // A word, acting as the key
        int value; // Value of the key-value pair
        enum Status status = EMPTY; // EMPTY, FILLED or REMOVED
    public:
        // The constructor and different methods for setting and getting variables. Some methods trigger change in status
        Node(){};
        void set_key_value(string key, int value){
            this->key = key;
            this->value = value;
            this->status = FILLED;
        }
        void remove(){
            this->status = REMOVED;
            this->key = "";
            this->value = 0;
            this->next = NULL;
            this->prev = NULL;
        }
        string get_key(){
            return this->key;
        }
        int get_value(){
            return this->value;
        }
        enum Status get_status(){
            return this->status;
        }
        pair<string, int> get_key_value(){
            return pair<string,int>(this->key, this->value);
        }
        Node* get_next(){
            return this->next;
        }
        Node* get_prev(){
            return this->prev;
        }
        void set_next(Node* next){
            this->next = next;
        }
        void set_prev(Node* prev){
            this->prev = prev;
        }
};

class HashTable{
    private:
        hash<string> str_hash; // The hash function (string->int) provided by the std library is fast and suitable for this case
        vector<Node> hash_table; // The hash table, containing each position a Node object
        Node* head = NULL; // Pointer to the most recently inserted/updated pair
        Node* tail = NULL; // Pointer to the least recently inserted/updated pair

    public:

        HashTable(size_t size){
            this->hash_table.resize(size, Node()); // We initialize the hash table with a fixed size and empty nodes
        }

        void insert(string key, int value){
            size_t idx = this->str_hash(key) % this->hash_table.size(); 
            size_t first_removed = this->hash_table.size(); 

            // In case of collision, we look in the next slots. We remember the first removed slot.
            while (true) {
                enum Status s = this->hash_table[idx].get_status();
                if (s == EMPTY) break;
                if (s == REMOVED) {
                    if (first_removed == this->hash_table.size()) first_removed = idx;
                } else if (s == FILLED) { // We update it
                    if (this->hash_table[idx].get_key() == key) {
                        Node* next = this->hash_table[idx].get_next();
                        Node* prev = this->hash_table[idx].get_prev();
                        if (next) next->set_prev(prev);
                        if (prev) prev->set_next(next);
                        // We modify the double linked list
                        if (&this->hash_table[idx] == this->head) this->head = next;
                        if (&this->hash_table[idx] == this->tail) this->tail = prev;
                        this->hash_table[idx].set_key_value(key, value);
                        this->hash_table[idx].set_prev(NULL);
                        this->hash_table[idx].set_next(this->head);
                        if (this->head) this->head->set_prev(&this->hash_table[idx]);
                        this->head = &this->hash_table[idx];
                        if (!this->tail) this->tail = &this->hash_table[idx];
                        return;
                    }
                }
                idx = (idx + 1) % this->hash_table.size();
            }
            
            // If possible, we insert in the first removed slot
            size_t slot = (first_removed != this->hash_table.size()) ? first_removed : idx;
            this->hash_table[slot].set_key_value(key, value);
            this->hash_table[slot].set_prev(NULL);
            this->hash_table[slot].set_next(this->head);
            if (this->head) this->head->set_prev(&this->hash_table[slot]);
            this->head = &this->hash_table[slot];
            if (!this->tail) this->tail = &this->hash_table[slot];
        }

        void remove(string key){
            size_t idx = str_hash(key) % this->hash_table.size(); 
            while(this->hash_table[idx].get_key() != key && 
                this->hash_table[idx].get_status() != EMPTY) idx = (idx+1)%this->hash_table.size(); // We search for the key, but it might not be present
            if(this->hash_table[idx].get_status() == EMPTY){
                cout << key << " not found\n";
                return;
            }
            // We remove the pair from the double linked list
            Node* next = this->hash_table[idx].get_next();
            Node* prev = this->hash_table[idx].get_prev();
            if (prev) prev->set_next(next); else head = next;
            if (next) next->set_prev(prev); else tail = prev;
            this->hash_table[idx].remove();
        }

        int get(string key){
            int idx = str_hash(key) % this->hash_table.size();
            while(this->hash_table[idx].get_key() != key && 
                this->hash_table[idx].get_status() != EMPTY) idx = (idx+1)%this->hash_table.size(); // We must skip removed pairs due to collisions
            if(this->hash_table[idx].get_status() == EMPTY){
                cout << key << " not found\n";
                return -1;
            }
            return this->hash_table[idx].get_value();
        }

        pair<string, int> get_last(){
            if(head) return head->get_key_value();
            else return pair<string,int>("",-1);
        }

        pair<string,int> get_first(){
            if(tail) return tail->get_key_value();
            else return pair<string,int>("",-1);
        }
};       

void show_help(){
    cout << "Introduce any of the following options:\n" <<
            "get <key>\n" <<
            "insert <key> <value>\n" <<
            "remove <key>\n" <<
            "get_last\n" <<
            "get_first\n" <<
            "help\n";
}

void process_input(string& input,HashTable& hash_table){
    if (input == "help") {
        show_help();
        return;
    }
    else if (input == "get") {
        string key;
        if (!(cin >> key)){ 
            cout << "ERR: missing <key>\n"; 
            return;
        }
        int value = hash_table.get(key);
        if (value != -1) cout << value << "\n";
    }

    else if (input == "insert") {
        string key; int value;
        if (!(cin >> key >> value)) { 
            cout << "ERR: usage: insert <key> <value>\n"; 
            return; 
        }
        hash_table.insert(key, value);
    }
    else if (input == "remove") {
        string key;
        if (!(cin >> key)) { 
            cout << "ERR: missing <key>\n"; 
            return; 
        }
        hash_table.remove(key);
    }
    else if (input == "get_first") {
        pair<string,int> kv = hash_table.get_first();
        if (kv.second !=-1) cout << kv.first << ": " << kv.second << "\n";
        else cout << "Not found\n";
    }
    else if (input == "get_last") {
        pair<string,int> kv = hash_table.get_last();
        if (kv.second !=-1) cout << kv.first << ": " << kv.second << "\n";
        else cout << "Not found\n";
    }
    else {
        cout << "Unknown command. Type 'help'.\n";
    }

}
int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <path-to-text-file>\n";
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }

    string word;
    HashTable hash_table(pow(2,18));
    int i = 0;
    // For simplicity, we just consider whitespaces, but we don't normalize the words or remove other characters.
    // For the value, as it's not stated in the problem, we will use an increasing counter
    while (file >> word) {
        hash_table.insert(word, i);
        i++;
    }
    string input;
    show_help();
    while(true){
        cout << "> " << flush;
        if (!(cin >> input)) break;
        process_input(input, hash_table);
    }
    return 0;
};
