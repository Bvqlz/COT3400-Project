#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <cstring>
#include <cstdio>   
#include <string>
#include <algorithm>

// one edge in a vertex's adjacency list
// the from vertex is whichever list this node sits in
struct VertexListNode {
    int toIndex; // this holds the index of the vertex this edge points to
    char Heading[20];
    double Distance; 
    double Speed; 
    VertexListNode* next;
};

struct GraphNode {
    char IntersectionName[80];
    VertexListNode* AdjVertexList; // head of this vertex's edge list
    VertexListNode* AdjVertexTail; // last node in the list so appending does not need a walk
};

// fields for each edge in fgcu_campus.csv 
struct EdgeRecord {
    char From[80];
    char To[80];
    char Heading[20];
    double Distance; 
    double Speed;
};

// parses one line of the csv into out with a single sscanf call
// returns false for blank or malformed lines so the caller can just skip them
// a simple parse like this will fail on negative distances, empty fields, or other malformed data. It is assumed that the input data is well formatted. 
bool parseLine(const char* line, EdgeRecord& out) {
    // the leading space skips any whitespace before the first quote
    // \" matches a literal quote and the commas match literal commas
    // %79[^\"] reads up to 79 chars that are not a quote
    // 79 leaves room for the '\0' in an 80 char array and 19 does the same for Heading
    // %lf reads a double for Distance and Speed
    int n = sscanf(line, " \"%79[^\"]\",\"%79[^\"]\",\"%19[^\"]\",%lf,%lf", out.From, out.To, out.Heading, &out.Distance, &out.Speed);

    // sscanf returns how many fields it filled so 5 means a full line
    // a blank line returns EOF and a bad line stops early with fewer than 5
    if(n != 5) {
        return false;
    }

    return true;
}

// removes leading and trailing spaces, tabs and \r from a line of user input
// the \r can show up when input comes from a file with Windows line endings
std::string trimLine(const std::string& text) {
    int start = 0;
    int end = (int)text.size() - 1;

    // move start forward past any blanks at the front
    while(start <= end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\r')) {
        start++;
    }

    // move end backward past any blanks at the back
    while(end >= start && (text[end] == ' ' || text[end] == '\t' || text[end] == '\r')) {
        end--;
    }

    return text.substr(start, end - start + 1); // an all blank line gives an empty string
}


class Graph {
    private:
        static const int MAX_VERTICES = 100; // the most vertices graph[] can hold
        GraphNode graph[MAX_VERTICES]; // this holds the various vertices of the graph.
        int vertexCount = 0; // keeps track of the number of vertices in the graph 
        std::vector<std::string> intersectionNames; // holds sorted intersection names for binary search
        std::vector<double> distances; // holds the distances from the start vertex to each vertex (of a particular route search)
        std::vector<int> previous; // holds the previous vertex index for each vertex in the shortest path (of a particular route search)

    public:
        // deconstructor 
        ~Graph() {
            for(int i = 0; i < vertexCount; i++) {
                VertexListNode* current = graph[i].AdjVertexList;
                while(current) {
                    VertexListNode* nextNode = current->next; // save next before deleting the node that holds it
                    delete current;
                    current = nextNode;
                }
                graph[i].AdjVertexList = nullptr;
                graph[i].AdjVertexTail = nullptr;
            }
        }

        void processFile(const char* fileName) {
            FILE* txt;
            char line[256];
            txt = fopen(fileName, "r");
            if(txt == nullptr) {
                std::cout << "could not open file." << std::endl;
                return;
            }

            while(fgets(line, sizeof(line), txt)) {
                EdgeRecord record;

                // parseLine does all the work of pulling the quoted fields apart. blank/bad lines are skipped
                if(!parseLine(line, record)) {
                    continue;
                }

                // keep both ends of the edge so a place that only shows up as a To still becomes a vertex
                // this gives 2 names per line and duplicates are removed after sorting
                intersectionNames.push_back(std::string(record.From)); // convert c-style string to std::string and add to vector
                intersectionNames.push_back(std::string(record.To));
            }

            fclose(txt);
        }

        std::vector<std::string>& getIntersectionNames() {
            return intersectionNames;
        }

       void Merge(std::vector<std::string>& vect, int leftFirst, int leftLast, int rightLast) {
            std::vector<std::string> sorted;
            int leftIndex = leftFirst;
            int rightIndex = leftLast + 1;

            while(leftIndex <= leftLast && rightIndex <= rightLast) {
                if(vect.at(leftIndex) <= vect.at(rightIndex)) {
                    //implies that the left subarray comes alphabetically before the right subarray
                    sorted.push_back(vect.at(leftIndex));
                    leftIndex++;
                }
                else {
                    sorted.push_back(vect.at(rightIndex));
                    rightIndex++;
                }
            }

            // need to loop through any remaining elements
            while(leftIndex <= leftLast) {
                sorted.push_back(vect.at(leftIndex));
                leftIndex++;
            }

            while(rightIndex <= rightLast) {
                sorted.push_back(vect.at(rightIndex));
                rightIndex++;
            }

            for(int i = 0; i < sorted.size(); i++) {
                vect.at(leftFirst + i) = sorted[i];
            }
        }

        void MergeSort(std::vector<std::string>& vect, int startIndex, int endIndex) {
            //when pass the intersectionNames vector to this function, we are passing it by reference.
            // this means that any changes made to vect inside this function will directly affect the original intersectionNames vector in the Graph class.
            // since it is passed by reference, we can think of vect as an alias for intersectionNames within this function.
            if(startIndex < endIndex) {
                int middle = (startIndex + endIndex) / 2;
                MergeSort(vect, startIndex, middle);
                MergeSort(vect, middle + 1, endIndex);

                Merge(vect, startIndex, middle, endIndex);
            }
        }

        int binarySearch(const char* intersectionName) {
            int left = 0;
            int right = vertexCount - 1; // or we can use intersectionNames.size() - 1

            while(left <= right) {
                int mid = left + (right - left) / 2;

                // compare once and reuse the result
                int cmp = strcmp(graph[mid].IntersectionName, intersectionName);

                if(cmp == 0) {
                    return mid;
                }
                else if(cmp < 0) {
                    //remember strcmp returns negative if first string comes before second string alphabetically
                    left = mid + 1; // we move the left index to the right of mid. 
                }
                else {
                    //this means that the strcmp returned a positive meaning that the first string came after the second string alphabetically
                    right = mid - 1; // we move the right index to the left of mid.
                }
            }

            return -1; // return -1 if not found
        }

        // removes repeated names from the sorted intersectionNames in one pass
        // keep is how many unique names we have so far and where the next one goes
        void removeDuplicates() {
            int keep = 0;

            for(int i = 0; i < (int)intersectionNames.size(); i++) {
                // only keep a name if it differs from the last one we kept
                // this works because the sort put equal names next to each other
                if(keep == 0 || intersectionNames[i] != intersectionNames[keep - 1]) {
                    intersectionNames[keep] = intersectionNames[i];
                    keep++;
                }
            }

            intersectionNames.resize(keep); // drop the leftover slots at the end
        }

        // adds vertices to the graph from the sorted intersection names
        void addVerticies() {
            // graph[] has a fixed size
            if((int)intersectionNames.size() > MAX_VERTICES) {
                std::cout << "Error too many locations (" << intersectionNames.size() << ") the limit is " << MAX_VERTICES << std::endl;
                return;
            }

            for(int i = 0; i < (int)intersectionNames.size(); i++) {
                strcpy(graph[i].IntersectionName, intersectionNames[i].c_str()); // changes our string to a char array
                graph[i].AdjVertexList = nullptr;
                graph[i].AdjVertexTail = nullptr; // empty list so there is no tail yet
            }

            vertexCount = intersectionNames.size(); // V is the number of unique names
        }

        // builds the sorted vertex list V from the file
        // sorting has to happen before removeDuplicates so equal names sit side by side
        void buildVertexList(const char* fileName) {
            processFile(fileName);
            MergeSort(intersectionNames, 0, (int)intersectionNames.size() - 1);
            removeDuplicates();
            addVerticies();
        }

        void addEdges(const char* fileName) {
            FILE* txt;
            char line[256];
            txt = fopen(fileName, "r");
            if(txt == nullptr) {
                std::cout << "could not open file." << std::endl;
                return;
            }

            while(fgets(line, sizeof(line), txt)) {
                EdgeRecord record;

                if(!parseLine(line, record)) {
                    continue; // skip blank or malformed lines
                }

                const char* fromIntersection = record.From;
                const char* toIntersection = record.To;
                const char* heading = record.Heading;
                double distance = record.Distance;
                double speed = record.Speed;

                int fromIndex = binarySearch(fromIntersection);
                int toIndex = binarySearch(toIntersection);

                if(fromIndex == -1 || toIndex == -1) {
                    std::cout << "An invalid intersection was found" << std::endl;
                    continue;
                }

                //create a new VertexListNode
                VertexListNode* newNode = new VertexListNode();
                newNode->toIndex = toIndex; // the destination name comes from graph using toIndex
                strcpy(newNode->Heading, heading);
                newNode->Distance = distance;
                newNode->Speed = speed;
                newNode->next = nullptr;

                //always check if the linked list is empty
                if(graph[fromIndex].AdjVertexList == nullptr) {
                    // the first node is both the head and the tail
                    graph[fromIndex].AdjVertexList = newNode;
                    graph[fromIndex].AdjVertexTail = newNode;
                }
                else {
                    // append through the tail pointer so each insert is constant time
                    // adding at the end keeps the edges in file order
                    graph[fromIndex].AdjVertexTail->next = newNode;
                    graph[fromIndex].AdjVertexTail = newNode;
                }

            }

            fclose(txt);
        }

        void dijkstra(int startIndex, bool useFastestPath) {
            distances.assign(vertexCount, INT_MAX); // sets all distances to inf
            previous.assign(vertexCount, -1); // sets all previous nodes to -1 (undefined) will be used to reconstruct the path later
            // the parameters here mean for the priority queue mean that we are storing pairs of (distance, vertexIndex)
            // the greater means that the priority queue will function as a min-heap based on the first element of the pair (distance)
            // as a reminder, in a min-heap the element with the smallest value has the highest priority and will be at the top of the heap. or that its children cannot be smaller than the parent node.
            std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> prioQueue;

            distances[startIndex] = 0;
            prioQueue.push({0, startIndex}); // we need this to kick off the algorithm

            while(!prioQueue.empty()) {
                double currentDistance = prioQueue.top().first;
                int currentVertex = prioQueue.top().second;
                prioQueue.pop();

                // if we have already found a better path to currentVertex, skip processing
                // all this does is that if we pop an element from the priority queue that has a distance greater than the currently known shortest distance to that vertex, we skip processing it.
                if(currentDistance > distances[currentVertex]) {
                    continue;
                }

                VertexListNode* edge = graph[currentVertex].AdjVertexList; // this gets the head of the adjacency list for the current vertex
                while(edge) {
                    double weight = useFastestPath ? (edge->Distance / edge->Speed) : edge->Distance;
                    double newDistance = currentDistance + weight; // currentDistance is the distance to the currentVertex. This weight is the distance from currentVertex to neighborIndex
                    if(newDistance < distances[edge->toIndex]) {
                        distances[edge->toIndex] = newDistance; // we update to the shorter distance
                        previous[edge->toIndex] = currentVertex; // we know that the we reached neighborIndex from currentVertex
                        prioQueue.push({newDistance, edge->toIndex}); // add the neighbor to the priority queue for further exploration
                    }

                    edge = edge->next; // move to the next neighbor in the adjacency list
                }

            }
        }

        void printDirections(int startIndex, int endIndex, bool showTime) {
            double totalDistance = 0;
            double totalTime = 0;

            if(distances[endIndex] == INT_MAX) {
                std::cout << "No path found from " << graph[startIndex].IntersectionName << " to " << graph[endIndex].IntersectionName << std::endl;
                return;
            }

            std::vector<int> path;
            int current = endIndex; // we start from the end index and trace back to the start index using the previous vector
            // -1 is the starting value that indicates no predecessor this would be at the start vertex
            while(current != -1) {
                path.push_back(current);
                current = previous[current]; // using the index of the previous vertex to trace back the path
            }

            std::reverse(path.begin(), path.end());

            std::cout << "From " << graph[startIndex].IntersectionName << std::endl;
            
            for(int i = 1; i < path.size(); i++) {
                int fromVertex = path[i - 1];
                int toVertex = path[i];

                VertexListNode* edge = graph[fromVertex].AdjVertexList;
                while(edge) {
                    
                    if(edge->toIndex == toVertex) {
                        std::cout << " Take " << edge->Heading << " to " << graph[edge->toIndex].IntersectionName << " " << edge->Distance << std::endl;

                        totalDistance += edge->Distance;
                        totalTime += edge->Distance / edge->Speed;
                        break;
                    }

                    edge = edge->next; // what this does is that it iterates through the adjacency list of fromVertex to find the edge leading to toVertex.
                }
            }

            std::cout << "\nTotal distance: " << totalDistance << " miles" << std::endl;
            std::cout << "Total time: " << (totalTime * 60) << " minutes" << std::endl;
            std::cout << "Route priority: " << (showTime ? "Fastest Time" : "Shortest Distance") << std::endl;
        }

        // asks for a location until the name matches a known vertex
        // returns the vertex index or -1 if the user quits or input runs out
        // allowQuit lets q or an empty line end the session from this prompt
        int promptForLocation(const char* label, bool allowQuit) {
            while(true) {
                std::cout << label;

                // getline reads the whole line so names with spaces work
                std::string input;
                if(!std::getline(std::cin, input)) {
                    return -1; // end of input so stop asking
                }
                input = trimLine(input);

                if(allowQuit && (input.empty() || input == "q")) {
                    return -1;
                }

                if(input.empty()) {
                    std::cout << "Please enter a location" << std::endl;
                    continue;
                }

                int index = binarySearch(input.c_str());
                if(index != -1) {
                    return index;
                }

                // keep asking until the name matches a known location
                std::cout << "Unknown location '" << input << "', try again" << std::endl;
            }
        }

        // asks for the route type until the user enters 0 or 1
        // returns 0 for shortest and 1 for fastest or -1 if input runs out
        int promptForRouteType() {
            while(true) {
                std::cout << "Choose path type (0 shortest distance, 1 fastest time): ";

                std::string input;
                if(!std::getline(std::cin, input)) {
                    return -1; // end of input so stop asking
                }
                input = trimLine(input);

                if(input == "0") {
                    return 0;
                }
                if(input == "1") {
                    return 1;
                }

                std::cout << "Please enter 0 or 1" << std::endl;
            }
        }

        // runs route queries until the user quits or input runs out
        void userInput() {
            while(true) {
                int startIndex = promptForLocation("Enter starting intersection (q to quit): ", true);
                if(startIndex == -1) {
                    return;
                }

                int endIndex = promptForLocation("Enter destination intersection: ", false);
                if(endIndex == -1) {
                    return;
                }

                int pathType = promptForRouteType();
                if(pathType == -1) {
                    return;
                }

                // a route to the same place has nothing to compute
                if(startIndex == endIndex) {
                    std::cout << "Start and destination are the same location" << std::endl << std::endl;
                    continue;
                }

                dijkstra(startIndex, pathType);
                printDirections(startIndex, endIndex, pathType);
                std::cout << std::endl; // blank line before the next query
            }
        }
};

int main(int argc, char* argv[]) {
    // the map file can be passed on the command line (e.g. ./router other_map.csv), otherwise we use the default
    const char* fileName = (argc > 1) ? argv[1] : "fgcu_campus.csv";

    Graph graph;
    graph.buildVertexList(fileName);
    graph.addEdges(fileName);
    graph.userInput();

    return 0;
}