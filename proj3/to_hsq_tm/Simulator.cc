#include "Simulator.h"
#include "RoutingProtocolImpl.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <string.h>

Simulator * sim;

int main(int argc, char **argv) {

//	freopen("C:\\Users\\CuiH\\Desktop\\test_cases\\simpletest5.out.my", "w", stdout);

	if (argc != 3) {
		printf("Usage: %s config_file_name DV|LS\n", argv[0]);
		return -1;
	}

	sim = new Simulator(argv[1]);

	sim->init(argv[2]);
	sim->init_routing_protocol(sim->protocol_type);
	sim->run();
	sim->cleanup();
}

unsigned int Simulator::time() {
	return global_time; // change to ms
}

void Simulator::set_alarm(RoutingProtocol *r, unsigned int duration, void *d) {
	Event *e;
	e = new Event_Alarm(r, d, global_time + duration, protocol_table[(char *) r]);
	event_q.push(e);
}

void Simulator::init_routing_protocol(eProtocolType type) {
	RoutingProtocol *rp;

	for (std::unordered_map<int, Node *>::iterator it = node_table.begin(); it != node_table.end(); it++) {

		rp = new RoutingProtocolImpl(it->second);
		protocol_table[(char *) rp] = it->second->id;
		it->second->rp = rp;
		rp->init(it->second->link_vector.size(), it->second->id, type);
	}
}

/*Reads the file and populates the data structures of the Simulator and fills up the event queue */
void Simulator::init(char *ptype) {

	global_time = 0;

	if (!strcmp(ptype, "DV")) {
		// DV
		sim->protocol_type = P_DV;
	} else {
		// LS
		sim->protocol_type = P_LS;
	}

	sim->stop_time = 0; // will never stop

	cout<<"Step 1: Begin Simulator Initialization...\n";

	// initialize the random number generator
	srand(0);

	//pyuan
	ifstream fstream(filename);
	string line;
	size_t pos, pos2;
	int nodeid;
	Node* n;

	/* read lines from the configuration file and process it */
	while(getline(fstream, line)) {
		if (!trim(line).empty())
			break;
	}
	/* reading contents here */
	if (line.find('[') != string::npos )// find configuration title
	{
		//if node definitions
		if (line.find("nodes") != string::npos)	{
			getline (fstream, line);
			line = trim(line);
			while (line.find('[')== string::npos) {
				while (!line.empty()) {
					pos = line.find(" ");
					if (pos != string::npos) {
						int nodeid = atoi(line.substr(0, pos).c_str());
						//cout << nodeid << endl;
						line.erase(0,pos+1);
						n = new Node(nodeid);
						add_node(n);
					}
					else {
						nodeid = atoi(trim(line).c_str());
						n = new Node(nodeid);
						add_node(n);
						break;
					}
				} // while
				getline (fstream, line);
				line = trim(line);
				if (line.empty())
					continue;
			} // while
		}//if types

		// find links now
		if (line.find("links")!= string::npos) {
			while (getline (fstream, line)) {
				line = trim(line);
				if (line.empty())
					continue;

				if (line.find('[')!= string::npos) //stopped if find another "[]"
					break;
				line = trim (line);

				if (line.find("(") != 0 ) {
					cout << "Error: wrong format in link definition \n";
					exit (-1);
				}

				if((pos=line.find(")")) == string::npos) {
					cout << "Error: wrong format in link definition \n";
					exit (-1);
				}

				string link = trim(line.substr(1, pos-1));

				//get link info
				if ((pos2 = link.find(","))== string::npos)
				{
					cout << "Error: wrong format in link definition \n";
					exit (-1);
				}
				int link_from = atoi(trim(link.substr(0,pos2)).c_str());
				int link_to = atoi(trim(link.substr(pos2+1,link.length())).c_str());

				line.erase(0, pos+1);
				line = trim(line);

				//delay setup
				double delay;
				if((pos = line.find("delay"))!= string::npos) {
					line = trim(line.erase(0,5));
					pos2 = line.find(" ");
					delay = atof(trim(line.substr(0,pos2)).c_str());
					line=trim(line.erase(0,pos2));
				}
				else
					delay = DEFAULT_DELAY;

				//cost setup
				int cost;

				if((pos = line.find("cost"))!= string::npos) {
					line = trim(line.erase(0,4));
					pos2 = line.find(" ");
					cost = atoi(trim(line.substr(0,pos2)).c_str());
					line=trim(line.erase(0,pos2));
				}
				else
					cost = DEFAULT_COST;
				//prob setup
				double prob;
				if((pos = line.find("prob"))!= string::npos) {
					line = trim(line.erase(0,4));
					//supose to be the end of line
					prob = atof(trim(line.substr(0,line.length())).c_str());
				}
				else
					prob = DEFAULT_PROB;

				//temp
				printf ("link: %d %d %d %f\n", link_from, link_to, (int) (delay * 1000), prob);
				//************calling up -- fixe me -- needs to be fixed by you guys
				//node.addlink -- and error check ...
				Link * lnk = new Link(node_table[link_from], node_table[link_to], (unsigned int) (delay * 1000), prob, cost);
				struct int_pair * pr = new int_pair(link_from, link_to);
				link_table[*pr] = lnk;
				node_table[link_from]->link_vector.push_back(lnk);
				node_table[link_to]->link_vector.push_back(lnk);


			}//while
		}//end of if "[links"]

		if (line.find("events")!= string::npos) {
			cout<<"Reading events...\n";
			while (getline (fstream, line)) {
				line = trim(line);
				if (line.empty())
					continue;

				if (line.find('[')!= string::npos)
					break;

				pos = line.find(" ");
				double time = atof(line.substr(0, pos).c_str()) * 1000;

				Event *e;
				string temp;
				//events
				cout<<"Line being read is: "<<line<<endl;
				if ((pos=line.find("linkdying"))!= string::npos) //linkdying
				{

					temp = trim(line.substr(pos+9, line.length()-pos-9));
					pos = temp.find(",");
					int src = atoi(trim(temp.substr(temp.find("(")+1,pos-1)).c_str());
					int dst = atoi(trim(temp.substr(pos+1,temp.find(")")-pos-1)).c_str());

					struct int_pair * intpr = new int_pair(src, dst);
					e = new Event_Link_Die(link_table[*intpr], (unsigned int) time);
					delete intpr;
				}
				else if ((pos=line.find("linkcomingup"))!= string::npos) //linkcomingup
				{

					temp = trim(line.substr(pos+12, line.length()-pos-12));
					pos = temp.find(",");
					int src = atoi(trim(temp.substr(temp.find("(")+1,pos-1)).c_str());
					int dst = atoi(trim(temp.substr(pos+1,temp.find(")")-pos-1)).c_str());

					struct int_pair * intpr = new int_pair(src, dst);
					e = new Event_Link_Come_Up(link_table[*intpr], (unsigned int) time);
					delete intpr;
				}

				else if ((pos=line.find("xmit"))!= string::npos) //xmit
				{

					temp = trim(line.substr(pos+4, line.length()-pos-4));
					pos = temp.find(",");
					unsigned short src = atoi(trim(temp.substr(temp.find("(")+1,pos-1)).c_str());
					unsigned short dst = atoi(trim(temp.substr(pos+1,temp.find(")")-pos-1)).c_str());
					struct sim_pkt_header * pkt = (struct sim_pkt_header *) malloc(sizeof(struct sim_pkt_header));
					pkt->type = (unsigned char) DATA;
					pkt->size = htons(sizeof(struct sim_pkt_header));
					pkt->src = htons(src);
					pkt->dst = htons(dst);
					e = new Event_Xmit_Pkt_End_To_End_Generic(node_table[src],node_table[dst], pkt, sizeof(struct sim_pkt_header), (unsigned int) time);
				}
				else if ((pos=line.find("changedelay"))!= string::npos) //changedelay
				{
					temp = trim(line.substr(pos+4, line.length()-pos-4));
					pos = temp.find(",");
					pos2 = temp.find(")");
					unsigned short src = atoi(trim(temp.substr(temp.find("(")+1,pos-1)).c_str());
					unsigned short dst = atoi(trim(temp.substr(pos+1,temp.find(")")-pos-1)).c_str());
					struct int_pair * intpr = new int_pair(src, dst);
					unsigned int newdelay = (unsigned int) (atof(temp.substr(pos2+1).c_str()) * 1000);
					e = new Event_Change_Delay(link_table[*intpr], newdelay, (unsigned int) time);
				}
				else if ((pos=line.find("end"))!= string::npos) //end
				{
					stop_time = (unsigned int) time;
					continue;
				}
				else //error
				{
					cout << "Error: events definition error - unrecogonized key words" << endl;
					exit(-1);
				}

				if (temp.find("(")== string::npos ||
					temp.find(")")== string::npos ||
					temp.find(",")== string::npos) {
					cout<<"Error:events configuration format error" << endl;
					exit(-1);
				}


				//*******need to check error and add events here **************
				//cout << time << "  " << " " << src << " " << dst << endl;
				event_q.push(e);
			}//while
		} //end of events configuration
	} // if	'['

	fstream.close();
	cout<<"Simulator Initialization complete.\n";
	return;
}

/*
 * trim: get rid of extra blank both at the beginning and end of the line
 */

string Simulator::trim (string s)
{
	if (s.empty())
		return s;

	int pos_t = s.find('\r');
	if (pos_t!=-1)
		s.erase(pos_t,1);

	int pos_b = s.find_first_not_of(" ");
	if (pos_b!=0)
		s.erase(0, pos_b);

	unsigned int pos_e = s.find_last_not_of(" ");
	if (pos_e!=s.size()-1)
		s.erase(pos_e+1, s.length());
	return s;
}

/* The main loop. Dequeue events and process them by calling dispatch on each event */
void Simulator::run() {
	cout<<"Step 2: Simulator beginning to run...\n";
	//cout<<"Event queue size is "<<event_q.size()<<endl;
	int i = 0;
	while(!event_q.empty()) {
		//    cout<<"iteration number "<<i<<" ,event q size = "<<event_q.size()<<" ";
		Event  *curr_event = event_q.top();
		//if (curr_event->time >= 1261.24) break;
		global_time = curr_event->time;
		cout<<"time = "<<global_time/1000.0<<" ";
		curr_event->pt();
		curr_event->dispatch();
		event_q.pop();
		delete curr_event;
		i++;
		if (stop_time != 0 && global_time > stop_time) {
			break;
		}
	}
	cout<<"Simulator has ended.\n";
	return;
}

/* The things that have to be done after run has finished including destroying any data structures, cleanup, etc.*/
void Simulator::cleanup() {
	//  cout<<"Step 3: Simulator Cleanup about to begin...\n";
	//  cout<<"Simulator cleanup successfully completed \n";
	return;
}

void Simulator::add_node(Node * node) {
	node_table[node->id] = node;
	//pyuan
	cout<<"node: "<<node->id<<"\n";
	return;
}
