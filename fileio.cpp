// Topology traversal

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct Path {
  int dest_port;
  int cost;
  string dest_id;
};

class RouterNode {
public:
  string id;
  int port;
  vector<Path> neighbors;
};

bool checkExist(vector<RouterNode>& nodes, string id) {
  for (int i = 0; i < nodes.size(); i++) {
    if (nodes[i].id == id)
      return true;
  }
  return false;
}

bool comp(const RouterNode a, const RouterNode b) {
  if (a.id < b.id)
    return true;
  return false;
}

int main(int argc, char *argv[]) {
  vector<RouterNode> nodes;
  string temp;
  
  if (argc != 2)
    cerr << "Incorrect no. of parameters";
  else {
    ifstream f(argv[1]);
    if (f.is_open()) {
      while (getline(f,temp)) {
        size_t i = temp.find(',');
        string s1 = temp.substr(0,i);
        temp = temp.substr(i+1);
        i = temp.find(',');
        string s2 = temp.substr(0,i);
        temp = temp.substr(i+1);
        i = temp.find(',');
        string s3 = temp.substr(0,i);
        string s4 = temp.substr(i+1);
        
        if (!checkExist(nodes, s2)) {
          RouterNode tempnode;
          tempnode.id = s2;
          tempnode.port = stoi(s3);
          nodes.push_back(tempnode);
        } else {
          for (int k = 0; k < nodes.size(); k++) {
            if (nodes[k].id == s2) {
              nodes[k].port = stoi(s3);
            }
          }
        }

        if (!checkExist(nodes, s1)) {
          RouterNode tempnode;
          tempnode.id = s1;
          nodes.push_back(tempnode);
        }
        
        for (int k = 0; k < nodes.size(); k++) {
          if (nodes[k].id == s1) {
            Path temppath;
            temppath.dest_id = s2;
            temppath.dest_port = stoi(s3);
            temppath.cost = stoi(s4);
            nodes[k].neighbors.push_back(temppath);
          }
        }
      }

      sort(nodes.begin(), nodes.end(), comp);
      
      for (int i = 0; i < nodes.size(); i++) {
        string filename = "details"+to_string(i+1)+".txt";
        ofstream ofile(filename);
        
        ofile << nodes[i].id << endl;
        ofile << nodes[i].port << endl;
        for (int j = 0; j < nodes[i].neighbors.size(); j++) {
          ofile << nodes[i].neighbors[j].dest_id << endl << nodes[i].neighbors[j].dest_port << endl << nodes[i].neighbors[j].cost << endl;
        }
        
        ofile.close();
      }
    }
    
    f.close();
  }
}