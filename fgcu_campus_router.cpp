#include <iostream>
#include <iomanip> 
#include <cmath>    
#include <vector>
#include <climits>
#include <cstring>
#include <cstdio>   
#include <string>

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


// one item in the MinHeap used by dijkstra
struct HeapEntry {
    double distance; // best known distance to vertex when this entry was added
    int vertex; // index into graph
};


class MinHeap {
    private:
        std::vector<HeapEntry> heapArray; // the heap stored level by level in an array

        // true if a should come out of the heap before b
        // smaller distance first and if the distances tie the smaller vertex first
        bool lessThan(const HeapEntry& a, const HeapEntry& b) {
            if(a.distance != b.distance) {
                return a.distance < b.distance;
            }
            return a.vertex < b.vertex;
        }

        void PercolateUp(int nodeIndex) {
            while(nodeIndex > 0) {
                // Compute the parent node's index
                int parentIndex = (nodeIndex - 1) / 2; // gets the parent index of this node

                // Check for a violation of the min heap property
                // if the current node is greater than or equal to the parent there is no violation
                if(!lessThan(heapArray[nodeIndex], heapArray[parentIndex])) {
                    // No violation so percolate up is done
                    return;
                }
                else {
                    // a child smaller than its parent breaks the min heap rule so swap them
                    HeapEntry temp = heapArray[nodeIndex];
                    heapArray[nodeIndex] = heapArray[parentIndex];
                    heapArray[parentIndex] = temp;

                    // keep going from the parent since it may still be too small for its own parent
                    nodeIndex = parentIndex;
                }
            }
        }

        void PercolateDown(int nodeIndex) {
            int heapSize = heapArray.size(); // how many entries are in the heap
            int childIndex = 2 * nodeIndex + 1; // formula to get the left child
            HeapEntry value = heapArray[nodeIndex]; // value of the current node

            // as long as the child index is within the heap size
            while(childIndex < heapSize) {
                // Find the min among the node and all the node's children
                HeapEntry minValue = value;
                int minIndex = -1;
                //for loop means that as long as there are two children we will check both
                for(int i = 0; i < 2 && i + childIndex < heapSize; i++) {
                    // Check if the child is smaller than the current min
                    if(lessThan(heapArray[i + childIndex], minValue)) {
                        // we update the min if we find a new min
                        minValue = heapArray[i + childIndex];
                        minIndex = i + childIndex;
                    }
                }

                // Check for a violation of the min heap property
                // if minIndex never changed then no child is smaller than the current node
                if(minIndex == -1) {
                    return;
                }
                else {
                    // there is a violation since one of the children is smaller than the current node
                    // Swap heapArray[nodeIndex] and heapArray[minIndex]
                    HeapEntry temp = heapArray[nodeIndex];
                    heapArray[nodeIndex] = heapArray[minIndex];
                    heapArray[minIndex] = temp;

                    // Continue loop from the min index node
                    // child node that was swapped is now the new node to check for further violations
                    nodeIndex = minIndex;
                    childIndex = 2 * nodeIndex + 1;
                }
            }
        }

    public:
        void Insert(HeapEntry value) {
            // Add the new value to the end of the array
            // push_back grows the vector when it is full so no ResizeArray is needed
            heapArray.push_back(value);

            // Percolate up from the last index to restore heap property
            // in other words we check if the newly added value violates the min heap property
            PercolateUp((int)heapArray.size() - 1);
        }

        // only call this when the heap is not empty
        HeapEntry Remove() {
            // Save the min value from the root of the heap
            HeapEntry minValue = heapArray[0];

            // Move the last item in the array into index 0
            // this just copies the value of the last element to the root
            HeapEntry replaceValue = heapArray[heapArray.size() - 1];
            heapArray.pop_back(); // this removes the last element from the heap
            if(!heapArray.empty()) {
                // since the heap is not empty we can replace the root
                heapArray[0] = replaceValue;

                // Percolate down to restore min heap property
                // down checks if the root's children are smaller than the root
                PercolateDown(0);
            }

            // Return the min value
            return minValue;
        }

        // looks at the smallest entry without removing it
        // only call this when the heap is not empty
        HeapEntry top() {
            return heapArray[0];
        }

        bool empty() {
            return heapArray.empty();
        }
};


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

            for(int i = 0; i < (int)sorted.size(); i++) {
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
            
            // the MinHeap stores entries of (distance, vertex)
            // the entry with the smallest distance is always at the root so Remove gives the closest vertex next
            // a vertex can sit in the heap more than once and the stale copies are skipped below
            MinHeap prioQueue;

            distances[startIndex] = 0;
            prioQueue.Insert({0, startIndex}); // we need this to start

            while(!prioQueue.empty()) {
                HeapEntry closest = prioQueue.Remove(); // takes the smallest distance out of the heap
                double currentDistance = closest.distance;
                int currentVertex = closest.vertex;

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
                        prioQueue.Insert({newDistance, edge->toIndex}); // add the neighbor to the heap for further exploration
                    }

                    edge = edge->next; // move to the next neighbor in the adjacency list
                }

            }
        }

        // flips the order of path in place
        // left and right move toward the middle and swap as they go
        void reversePath(std::vector<int>& path) {
            int left = 0;
            int right = (int)path.size() - 1;
            while(left < right) {
                int temp = path[left];
                path[left] = path[right];
                path[right] = temp;
                left++;
                right--;
            }
        }

        void printDirections(int startIndex, int endIndex, bool showTime) {
            double totalDistance = 0;
            double totalTime = 0;

            const char* routeType = showTime ? "Fastest" : "Shortest";

            // the destination was never reached so there is nothing to print
            if(distances[endIndex] == INT_MAX) {
                std::cout << "No " << (showTime ? "fastest" : "shortest") << " route from " << graph[startIndex].IntersectionName << " to " << graph[endIndex].IntersectionName << std::endl;
                return;
            }

            std::vector<int> path;
            int current = endIndex; // we start from the end index and trace back to the start index using the previous vector
            // -1 is the starting value that indicates no predecessor this would be at the start vertex
            while(current != -1) {
                path.push_back(current);
                current = previous[current]; // using the index of the previous vertex to trace back the path
            }

            reversePath(path); // the walk above gave end to start so flip it

            std::cout << routeType << " route, " << graph[startIndex].IntersectionName << " to " << graph[endIndex].IntersectionName << ":" << std::endl;

            // find the longest from name on this route so the arrows line up
            // the last vertex is never a from name so it is skipped
            int nameWidth = 0;
            for(int i = 0; i < (int)path.size() - 1; i++) {
                int length = strlen(graph[path[i]].IntersectionName);
                if(length > nameWidth) {
                    nameWidth = length;
                }
            }

            std::cout << std::fixed; // print a set number of decimals instead of the shortest form

            for(int i = 1; i < (int)path.size(); i++) {
                int fromVertex = path[i - 1];
                int toVertex = path[i];

                VertexListNode* edge = graph[fromVertex].AdjVertexList;
                while(edge) {

                    if(edge->toIndex == toVertex) {
                        // one leg in the order from then heading then distance then to
                        // both names come from graph using the vertex indices
                        std::cout << "  " << std::left << std::setw(nameWidth) << graph[fromVertex].IntersectionName
                                  << "  " << std::setw(3) << edge->Heading
                                  << std::right << std::setw(7) << std::setprecision(2) << edge->Distance << " m -> "
                                  << graph[toVertex].IntersectionName << std::endl;

                        totalDistance += edge->Distance;
                        totalTime += edge->Distance / edge->Speed; // distance over speed is already seconds so no conversion
                        break;
                    }

                    edge = edge->next; // what this does is that it iterates through the adjacency list of fromVertex to find the edge leading to toVertex.
                }
            }

            // round first so the minutes and seconds add up
            // lround rounds to the nearest whole second and returns an integer
            long roundedSeconds = std::lround(totalTime);
            long minutes = roundedSeconds / 60;
            long seconds = roundedSeconds % 60;

            std::cout << "Total distance: " << std::setprecision(2) << totalDistance << " m" << std::endl;
            std::cout << "Total time: " << std::setprecision(1) << totalTime << " s (" << minutes << " min " << seconds << " s)" << std::endl;

            std::cout << std::defaultfloat; // turn fixed back off so later output is not affected
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

        // q at the first prompt goes back to the menu
        void runRouteQuery() {
            int startIndex = promptForLocation("Enter starting intersection (q for menu): ", true);
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
                std::cout << "Start and destination are the same location" << std::endl;
                return;
            }

            dijkstra(startIndex, pathType);
            printDirections(startIndex, endIndex, pathType);
        }

        // reads the tour start and then 1 to 7 stops into places
        // places[0] is the start and places[1..k] are the stops in the order entered
        // returns false if the user backs out or input runs out
        bool readTourStops(std::vector<int>& places) {
            const int MAX_STOPS = 7;

            int start = promptForLocation("Enter tour start (q for menu): ", true);
            if(start == -1) {
                return false;
            }
            places.push_back(start);

            while((int)places.size() - 1 < MAX_STOPS) {
                std::string label = "Enter stop " + std::to_string(places.size()) + " (empty line to finish): ";
                int stop = promptForLocation(label.c_str(), true);

                if(stop == -1) {
                    if(!std::cin) {
                        return false; // end of input
                    }

                    // an empty line ends the list but a tour needs at least one stop
                    if(places.size() == 1) {
                        std::cout << "A tour needs at least 1 stop" << std::endl;
                        continue;
                    }
                    return true;
                }

                if(stop == start) {
                    std::cout << "That is the start location, pick a different stop" << std::endl;
                    continue;
                }

                // compare against every stop so far so no stop is visited twice
                bool repeated = false;
                for(int i = 1; i < (int)places.size(); i++) {
                    if(places[i] == stop) {
                        repeated = true;
                    }
                }
                if(repeated) {
                    std::cout << "That stop is already in the tour, pick a different one" << std::endl;
                    continue;
                }

                places.push_back(stop);
            }

            // the loop only gets here once 7 stops are in
            std::cout << "7 stops is the limit so no more stops are taken" << std::endl;
            return true;
        }

        // fills table[i][j] with the shortest distance from places[i] to places[j]
        // runs dijkstra once from each place and reads distances for the others
        // returns false if some place cannot reach another
        bool buildStopTable(const std::vector<int>& places, std::vector<std::vector<double>>& table) {
            int size = places.size();
            table.assign(size, std::vector<double>(size, 0));

            for(int i = 0; i < size; i++) {
                dijkstra(places[i], false); // false means shortest distance is the weight

                for(int j = 0; j < size; j++) {
                    // a distance still at infinity means dijkstra never reached that place
                    if(distances[places[j]] == INT_MAX) {
                        std::cout << "No route from " << graph[places[i]].IntersectionName << " to " << graph[places[j]].IntersectionName << " so the tour is cancelled" << std::endl;
                        return false;
                    }
                    table[i][j] = distances[places[j]];
                }
            }

            return true;
        }

        // recursive helper for bruteForceTour
        // fills order one position at a time with every stop not used yet
        // when all k positions are filled it scores that order against the best so far
        void tryAllOrders(const std::vector<std::vector<double>>& table, int k, int position,
            std::vector<bool>& used, std::vector<int>& order,
            std::vector<int>& bestOrder, double& bestTotal, long& ordersTried) {
            if(position == k) {
                // the tour starts and ends at row 0 which is the start location
                double total = table[0][order[0]];
                for(int i = 1; i < k; i++) {
                    total += table[order[i - 1]][order[i]];
                }
                total += table[order[k - 1]][0];

                ordersTried++;

                // strict less than keeps the first order found when two totals tie
                if(bestOrder.empty() || total < bestTotal) {
                    bestTotal = total;
                    bestOrder = order;
                }
                return;
            }

            // try every unused stop in the next position
            for(int stop = 1; stop <= k; stop++) {
                if(!used[stop]) {
                    used[stop] = true;
                    order[position] = stop;

                    tryAllOrders(table, k, position + 1, used, order, bestOrder, bestTotal, ordersTried);

                    used[stop] = false; // undo the choice so the next branch starts clean
                }
            }
        }

        // finds the shortest tour by trying all k factorial orders of the stops
        // bestOrder gets table indices 1..k in visiting order
        void bruteForceTour(const std::vector<std::vector<double>>& table, int k, 
            std::vector<int>& bestOrder, double& bestTotal, long& ordersTried) {
            std::vector<bool> used(k + 1, false); // used[s] is true while stop s is in the current order
            std::vector<int> order(k); // the order being built right now

            bestOrder.clear();
            bestTotal = 0;
            ordersTried = 0;

            tryAllOrders(table, k, 0, used, order, bestOrder, bestTotal, ordersTried);
        }

        // builds a tour by always walking to the closest stop not visited yet
        // ties go to the stop entered first so the result is repeatable
        void nearestNeighborTour(const std::vector<std::vector<double>>& table, int k, 
            std::vector<int>& order, double& total) {
            std::vector<bool> visited(k + 1, false);
            int current = 0; // row 0 is the start location

            order.clear();
            total = 0;

            for(int step = 0; step < k; step++) {
                // scan every unvisited stop and remember the closest one
                int closest = -1;
                for(int stop = 1; stop <= k; stop++) {
                    if(!visited[stop] && (closest == -1 || table[current][stop] < table[current][closest])) {
                        closest = stop;
                    }
                }

                visited[closest] = true;
                order.push_back(closest);
                total += table[current][closest];
                current = closest;
            }

            total += table[current][0]; // walk back to the start
        }

        // prints start then the stops in order then start again
        // order holds table indices so places turns them back into vertex indices
        void printTourOrder(const std::vector<int>& places, const std::vector<int>& order) {
            std::cout << graph[places[0]].IntersectionName;
            for(int i = 0; i < (int)order.size(); i++) {
                std::cout << ", " << graph[places[order[i]]].IntersectionName;
            }
            std::cout << ", " << graph[places[0]].IntersectionName << std::endl;
        }

        // prints the stops as entered then both tours with their totals and the ratio
        void printTour(const std::vector<int>& places,
            const std::vector<int>& bestOrder, double bestTotal, long ordersTried,
            const std::vector<int>& nearestOrder, double nearestTotal) {
            std::cout << "Tour from " << graph[places[0]].IntersectionName << ": ";
            for(int i = 1; i < (int)places.size(); i++) {
                if(i > 1) {
                    std::cout << ", ";
                }
                std::cout << graph[places[i]].IntersectionName;
            }
            std::cout << std::endl;

            std::cout << std::fixed << std::setprecision(2);

            std::cout << "Optimal: ";
            printTourOrder(places, bestOrder);
            std::cout << "  total " << bestTotal << " m (brute force: " << ordersTried << (ordersTried == 1 ? " order)" : " orders)") << std::endl;

            std::cout << "Nearest neighbor: ";
            printTourOrder(places, nearestOrder);
            std::cout << "  total " << nearestTotal << " m" << std::endl;

            // the ratio shows how far nearest neighbor is from the best tour
            // 1.000 means it found the optimal tour too
            std::cout << "Ratio: " << std::setprecision(3) << (nearestTotal / bestTotal) << std::endl;

            std::cout << std::defaultfloat; // turn fixed back off so later output is not affected
        }

        // runs one tour query from reading the stops to printing both tours
        void runTourQuery() {
            std::vector<int> places;
            if(!readTourStops(places)) {
                return;
            }

            std::vector<std::vector<double>> table;
            if(!buildStopTable(places, table)) {
                return;
            }

            int k = places.size() - 1; // number of stops not counting the start

            std::vector<int> bestOrder;
            double bestTotal;
            long ordersTried;
            bruteForceTour(table, k, bestOrder, bestTotal, ordersTried);

            std::vector<int> nearestOrder;
            double nearestTotal;
            nearestNeighborTour(table, k, nearestOrder, nearestTotal);

            printTour(places, bestOrder, bestTotal, ordersTried, nearestOrder, nearestTotal);
        }

        // menu loop that runs route or tour queries until the user quits or input runs out
        // every prompt uses getline so cin >> and getline never get mixed
        void userInput() {
            while(true) {
                std::cout << "Choose 1 for route, 2 for tour, q to quit: ";

                std::string choice;
                if(!std::getline(std::cin, choice)) {
                    return; // end of input
                }
                choice = trimLine(choice);

                if(choice == "q") {
                    return;
                }
                else if(choice == "1") {
                    runRouteQuery();
                }
                else if(choice == "2") {
                    runTourQuery();
                }
                else {
                    std::cout << "Please enter 1, 2 or q" << std::endl;
                    continue;
                }

                // a prompt that hit end of input leaves cin failed so stop here
                if(!std::cin) {
                    return;
                }
                std::cout << std::endl; // blank line before the next menu
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