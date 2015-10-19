#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::asio;

using namespace std;
using boost::asio::ip::udp;

struct RoutePath {
    string next_hop_id;
    int next_hop;
    int dest_port;
    int cost;

    RoutePath(string next_hop_id, int next_hop, int dest_port, int cost)
    {
        this->next_hop_id = next_hop_id;
        this->next_hop = next_hop;
        this->dest_port = dest_port;
        this->cost = cost;
    }
    RoutePath()
    {
        this->next_hop = 0;
        this->dest_port = 0;
        this->cost = INT32_MAX;
    }
};

class router {
public:
    void insertLink(string next_hop_id, int next_hop, int dest_port, int cost) {
        links.push_back(RoutePath(next_hop_id, next_hop, dest_port, cost));
    }
    
    int getLink(int dest_port, int &pos) {
        for(int i = 0; i < links.size(); i++) {
            if (links[i].dest_port == dest_port) {
                pos = i;
                return 0;
            }
        }
        return 1;
    }
    
    int getNeighborCost(int next_hop, int &pos) {
        for (int i = 0; i < neighbors.size(); i++) {
            if (neighbors[i].dest_port == next_hop) {
                pos = i;
                return 0;
            }
        }
        return 1;
    }
    
    void queueLink(int dest_port, int cost) {
        for (int i = 0; i < updates.size(); i++) {
            if (updates[i].dest_port == dest_port) {
                updates[i].cost = cost;
                return;
            }
        }
        updates.push_back(RoutePath(id, port, dest_port, cost));
    }
    
    void updateLinks(vector<RoutePath> paths) {
        for (int i = 0; i < paths.size(); i++) {
            int temp;
            int error = getLink(paths[i].dest_port, temp);
            int temp1;
            int error1 = getNeighborCost(paths[i].next_hop, temp1);

            if (error1 != 0) {
                continue;
            }
            if (paths[i].next_hop == paths[i].dest_port) {
                continue;
            }
            if (error == 0) {
                if (links[temp].cost > (paths[i].cost + neighbors[temp1].cost)) {
                    links[temp].next_hop_id = paths[i].next_hop_id;
                    links[temp].next_hop = paths[i].next_hop;
                    links[temp].cost = paths[i].cost + neighbors[temp1].cost;
                    queueLink(links[temp].dest_port, links[temp].cost);
                }
                if (links[temp].cost == (paths[i].cost + neighbors[temp1].cost) && links[temp].next_hop_id > paths[i].next_hop_id) {
                    links[temp].next_hop_id = paths[i].next_hop_id;
                    links[temp].next_hop = paths[i].next_hop;
                }
            }
            else {
                insertLink(paths[i].next_hop_id, paths[i].next_hop, paths[i].dest_port, paths[i].cost + neighbors[temp1].cost);
                queueLink(paths[i].dest_port, paths[i].cost + neighbors[temp1].cost);
            }
        }
    }
    
    string encodeUpdates() {
        string s = "Ln";
        for (int i = 0; i < updates.size(); i++) {
            s += (id + "n");
            s += (to_string(port) + "n");
            s += (to_string(updates[i].dest_port) + "n");
            s += (to_string(updates[i].cost) + "n");
        }
        s = s.substr(0, s.size() - 1);
        updates.clear();
        return s;
    }
    
    void decodeUpdates(string s) {
        of.open("routerOutput"+id+".txt", ios_base::app);
        if (s[0] == 'L') {
            s = s.substr(2, string::npos);
            stringstream ss(s);
            vector<RoutePath> newLinks;

            while (ss.good()) {
                string next_hop_id;
                ss >> next_hop_id;
                int next_hop;
                ss >> next_hop;
                int dest_port;
                ss >> dest_port;
                int cost;
                ss >> cost;
                if (dest_port == this->port) {
                    continue;
                }
                newLinks.push_back(RoutePath(next_hop_id, next_hop, dest_port, cost));
            }

            updateLinks(newLinks);
            printHeader("DistanceVectors From " + to_string(newLinks[0].next_hop));
            of << "Updated Tablen";
            printLinks();
            } else if (s[0] == 'S') {
            s = s.substr(2, string::npos);
            stringstream ss(s);
            int s_port;
            ss >> s_port;
            if (removePort(s_port)) {
                send("Sn"+s);
                links.clear();
                initializeLinks();
                printHeader("Signal From " + to_string(s_port));
                of << "(Dest Port, Cost, Next Hop Port)n";

                for (int i = 0; i < links.size(); i++) {
                    of << "(" << neighbors[i].dest_port << ", " << neighbors[i].cost << ", " << neighbors[i].next_hop <<")n";
                }
            }
        }
        of.close();
    }
    
    int removePort(int s_port) {
        int pos = -1;
        int flag = 0;
        if (!getNeighborCost(s_port, pos)) {
            neighbors.erase(neighbors.begin() + pos);
            flag = 1;
        }
        pos = -1;
        if (!getLink(s_port, pos)) {
            links.erase(links.begin() + pos);
            flag = 1;
        }
        for (vector<RoutePath>::iterator it = links.begin(); it != links.end(); ) {
            if (it->next_hop == s_port)
            it = links.erase(it);
            else
            it++;
        }
        return flag;
    }
    
    void initializeLinks() {
        updates.clear();
        for (int i = 0; i < neighbors.size(); i++) {
            insertLink(neighbors[i].next_hop_id, neighbors[i].dest_port, neighbors[i].dest_port, neighbors[i].cost);
            queueLink(neighbors[i].dest_port, neighbors[i].cost);
        }
    }
    
    void sendUpdates(const boost::system::error_code error) {
        if (updates.size() > 0)
        send(encodeUpdates());
        wait(1);
    }
    
    void wait(int sec) {
        t.expires_from_now(boost::posix_time::seconds(sec)); //repeat rate here
        t.async_wait(boost::bind(&router::sendUpdates, this, boost::asio::placeholders::error));
    }
    
    void cancel() {
        t.cancel();
    }
    
    void process_data(string data) {
        string src_port, dest_port, src_router, dest_router, message, next_hop_id;
        int dest_port_int, i;
        
        for (i = 2; data[i] != 'n'; i++)
        src_router += data[i];
        
        for (i++; data[i] != 'n'; i++)
        dest_router += data[i];
        
        for (i++; data[i] != 'n'; i++)
        dest_port += data[i];
        
        for (i++; data[i] != 'n'; i++)
        message += data[i];
        
        dest_port_int = stoi(dest_port);
        
        of.open("routerOutput"+id+".txt", ios_base::app);
        
        printHeader("Data");
        of << "Source router: " << src_router << endl;
        of << "Destination router: " << dest_router << endl;
        of << "UDP Port of arrival: " << port << endl;
        
        if (dest_port_int != port) {
            int next_hop_port;
            for (int i = 0; i < links.size(); i++) {
                if (links[i].dest_port == dest_port_int) {
                    next_hop_port = links[i].next_hop;
                    next_hop_id = links[i].next_hop_id;
                }
            }
            
            of << "UDP Port Forwarded To: " << next_hop_port << endl;
            forward_data(data, next_hop_port);
        } else {
          of << "Reached Destination. Message: " << message << endl;
        }

        of.close();
    }
    
    void printHeader(string messageType) {
        of << "-------New " << messageType << "-------n";
        of << "Time: " << boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()) << "n";
    }
    
    void printLinks() {
        of << "(Dest Port, Cost, Next Hop Port)n";
        for (int i = 0; i < links.size(); i++) {
            of << "(" << links[i].dest_port << ", " << links[i].cost << ", " << links[i].next_hop <<")n";
        }
    }
    
    router(boost::asio::io_service& io_service, short port, string router_id, vector<RoutePath>& nodes, boost::asio::signal_set& signals)
    : io_service_(io_service),
    socket_(io_service, udp::endpoint(udp::v4(), port)),
    id(router_id),
    neighbors(nodes),
    port(port),
    t(io_service),
    m(0) {
        signals.async_wait(boost::bind(&router::handle_signal, this, boost::asio::placeholders::error, _2));
        boost::asio::socket_base::reuse_address option(true);
        deadline_timer timer(io_service);
        socket_.set_option(option);
        socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&router::handle_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
        if (id != "H") {
            initializeLinks();
            of.open("routerOutput"+id+".txt");
            of << "Messages for " << id << "n";
            wait(1 + (port % 10000)/10);
            } else {
            //of.open("hostOutput.txt");
        }
        of.close();
    }
    
    ~router() {
        of.close();
    }
    
    void handle_signal(const boost::system::error_code& error, int signal_number) {
        if (!error) {
            string message = "Sn" + to_string(port) + "n";
            send(message);
            exit(1);
        }
    }
    
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
        if (!error && bytes_recvd > 0) {
            //cout << id << ":nMessage num: " << m << "n";
            m++;
            //cout << boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()) << "n";
            //cout << data_ << "n";
            string s(data_);
            bzero(data_, 1024);
            
            if (s[0] == 'M')
            process_data(s);
            else
            decodeUpdates(s);
            
            socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), sender_endpoint_,
            boost::bind(&router::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        } else {
            socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), sender_endpoint_,
            boost::bind(&router::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        }
    }
    
    void handle_send_to(const boost::system::error_code& error, size_t bytes_transferred) {
        socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        boost::bind(&router::handle_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
    
    void send(string message) {
        for (int i = 0; i < neighbors.size(); i++) {
            int dest_port = neighbors[i].dest_port;
            char arg1[10] = "localhost";
            string portStr = to_string(dest_port);
            udp::resolver resolver(io_service_);
            udp::resolver::query query(udp::v4(), arg1, portStr.c_str());
            udp::resolver::iterator iterator = resolver.resolve(query);
            socket_.async_send_to(
            boost::asio::buffer(message, message.size()), *iterator,
            boost::bind(&router::handle_send_to, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        }
    }
    
    void forward_data(string data, int forward_port) {
        char arg1[10] = "localhost";
        string portStr = to_string(forward_port);
        udp::resolver resolver(io_service_);
        udp::resolver::query query(udp::v4(), arg1, portStr.c_str());
        udp::resolver::iterator iterator = resolver.resolve(query);
        socket_.async_send_to(
        boost::asio::buffer(data, data.size()), *iterator,
        boost::bind(&router::handle_send_to, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    }
    
    boost::asio::io_service& io_service_;
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
    string id;
    int port;
    vector<RoutePath> neighbors;
    vector<RoutePath> links;
    vector<RoutePath> updates;
    deadline_timer t;
    ofstream of;
    int m;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: router <file>n" << argc;
            return 1;
        }
        
        int port = 0;
        string id;
        string temp;
        vector<RoutePath> nodes;
        string source_port, dest_port, source_router_id, dest_router_id;
        
        boost::asio::io_service io_service;
        ifstream f(argv[1]);
        if (f.is_open()) {
            getline(f,id);
            getline(f,temp);
            port = stoi(temp);
            if (id == "H") {
                getline(f, source_port);
                getline(f, dest_port);
                getline(f, source_router_id);
                getline(f, dest_router_id);
            }
            else {
                while (getline(f,temp)) {
                    RoutePath temppath;
                    temp.erase(temp.find_last_not_of(" nrt")+1);
                    temppath.next_hop_id = temp;
                    getline(f,temp);
                    temppath.dest_port = stoi(temp);
                    temppath.next_hop = stoi(temp);
                    getline(f,temp);
                    temppath.cost = stoi(temp);
                    nodes.push_back(temppath);
                }
            }
        }
        
        f.close();
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM, SIGABRT);
        router r(io_service, port, id, nodes, signals);
        
        if (id == "H") {
            string data_packet = "Mn"+ source_router_id + "n" + dest_router_id + "n" + dest_port + "nthis is a messagen";
            cout << "-------Sending Data Packet-------n";
            cout << data_packet;
            r.forward_data(data_packet, stoi(source_port));
            exit(0);
        }
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}