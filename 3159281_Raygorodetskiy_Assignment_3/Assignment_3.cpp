// Author : Anthony Raygorodetskiy
// KU ID: 3159281
// Brief Description: This is a C++ program that is meant to take in emails given 
// by a file and orders the emails based on importance (Ex: Boss > Subordinate) 
// and takes in commands such as COUNT to count how many emails are in the 
// heap stack, as well as going through the email stack whenever it reads an email. 
// Inputs: a .txt file with email strings as well as commands that either go through the email or 
// count how many emails are left in the heap
// Outputs: Terminal output of whichever emails are to be read, and counting how many unread emails
// are left 
// Creation Date: 9/31/2026
// Last Revised: 10/1/2026
// Sources/Collaboraters: Gemini and ChatGPT
#include <iostream>   // Include standard input/output stream library (std::cout, std::cin, std::cerr)
#include <fstream>    // Include file stream library for reading input files (std::ifstream)
#include <string>     // Include standard string class and string manipulation utilities
#include <vector>     // Include standard dynamic array container used as the heap backing store
#include <utility>    // Include utilities such as std::move and std::swap
#include <cstdint>    // Include exact-width integer types such as uint8_t and uint64_t

// ==========================================
// Enumeration: Category
// ==========================================
// Strongly-typed 1-byte enum mapping sender categories to numeric priority values
enum class Category : uint8_t {
    OtherPerson = 1,      // Lowest priority: read last
    ImportantPerson = 2,  // Priority 2
    Peer = 3,             // Priority 3
    Subordinate = 4,      // Priority 4
    Boss = 5,             // Highest priority: read first
    Unknown = 0           // Fallback value for unrecognized categories
};

// Converts an input string into its corresponding Category enum value
inline Category stringToCategory(const std::string& cat) {
    if (cat == "Boss") return Category::Boss;                       // Return Boss if string matches
    if (cat == "Subordinate") return Category::Subordinate;         // Return Subordinate if string matches
    if (cat == "Peer") return Category::Peer;                       // Return Peer if string matches
    if (cat == "ImportantPerson") return Category::ImportantPerson; // Return ImportantPerson if string matches
    if (cat == "OtherPerson") return Category::OtherPerson;         // Return OtherPerson if string matches
    return Category::Unknown;                                       // Return Unknown for any other string
}

// Converts a Category enum back into a printable C-style string for display
inline const char* categoryToString(Category cat) {
    switch (cat) {                                       // Check the category enum value
        case Category::Boss: return "Boss";              // Map Boss to string literal
        case Category::Subordinate: return "Subordinate";// Map Subordinate to string literal
        case Category::Peer: return "Peer";              // Map Peer to string literal
        case Category::ImportantPerson: return "ImportantPerson"; // Map ImportantPerson to string literal
        case Category::OtherPerson: return "OtherPerson";// Map OtherPerson to string literal
        default: return "Unknown";                       // Default fallback for Unknown or invalid values
    }
}

// ==========================================
// Class: Email
// ==========================================
// Represents an email message with precomputed priority keys
class Email {
private:
    Category category;       // Enum storing the sender category (1 byte)
    std::string subjectLine; // String storing the email subject
    std::string rawDate;     // Original date string formatted as MM-DD-YYYY
    int normalizedDate;      // Precomputed integer date (YYYYMMDD) for O(1) chronological comparison
    uint64_t sequenceId;     // Monotonically increasing ID preserving arrival order for tie-breaking

    // Parses "MM-DD-YYYY" into an integer YYYYMMDD with graceful error handling
    static int parseDate(const std::string& d) {
        if (d.length() < 10) return 0;                       // Validate minimum expected date length
        try {                                                // Protect against malformed numeric substrings
            int m = std::stoi(d.substr(0, 2));               // Extract two-digit month
            int day = std::stoi(d.substr(3, 2));             // Extract two-digit day
            int y = std::stoi(d.substr(6, 4));               // Extract four-digit year
            return y * 10000 + m * 100 + day;                // Combine into single integer: YYYYMMDD
        } catch (...) {                                      // Catch std::invalid_argument or std::out_of_range
            return 0;                                        // Fallback date on parsing failure
        }
    }

public:
    // Default constructor initializing all fields to empty/zero
    Email() : category(Category::Unknown), subjectLine(""), rawDate(""), normalizedDate(0), sequenceId(0) {}

    // Parameterized constructor transferring string ownership via std::move
    Email(Category cat, std::string subject, std::string date, uint64_t seq)
        : category(cat),                                     // Set category enum
          subjectLine(std::move(subject)),                   // Move subject string into member
          rawDate(std::move(date)),                          // Move raw date string into member
          normalizedDate(parseDate(rawDate)),                // Precompute date integer representation
          sequenceId(seq) {}                                 // Store arrival sequence ID

    // Less-than operator defining priority ordering for the MaxHeap
    bool operator<(const Email& other) const {
        if (category != other.category) {                    // Compare categories first
            return static_cast<uint8_t>(category) < static_cast<uint8_t>(other.category); // Higher rank has higher priority
        }
        if (normalizedDate != other.normalizedDate) {        // If categories match, compare dates
            return normalizedDate < other.normalizedDate;    // Newer date (larger integer) has higher priority
        }
        return sequenceId > other.sequenceId;                // Tie-breaker: earlier arrival (lower ID) has higher priority
    }

    // Greater-than operator implemented in terms of operator<
    bool operator>(const Email& other) const {
        return other < *this;                                // Invert arguments to evaluate greater-than
    }

    // Displays the email details formatted per assignment specifications
    void display() const {
        std::cout << "Sender: " << categoryToString(category) << "\n" // Print sender category
                  << "Subject: " << subjectLine << "\n"               // Print subject line
                  << "Date: " << rawDate << "\n";                     // Print raw date string
    }
};

// ==========================================
// Class Template: MaxHeap
// ==========================================
// Custom generic list-based max heap implementation
template <typename T>
class MaxHeap {
private:
    std::vector<T> data;                                     // Contiguous storage buffer backing the binary tree

    int parent(int i) const { return (i - 1) / 2; }          // Compute zero-based parent index
    int left(int i) const { return 2 * i + 1; }              // Compute zero-based left child index
    int right(int i) const { return 2 * i + 2; }             // Compute zero-based right child index

    // Restores heap order upward from the given element index
    void siftUp(int index) {
        while (index > 0 && data[index] > data[parent(index)]) { // While node is greater than its parent
            std::swap(data[index], data[parent(index)]);          // Swap node with its parent
            index = parent(index);                                // Move current index up to parent position
        }
    }

    // Iteratively restores heap order downward from the given index (O(1) auxiliary space)
    void siftDown(int index) {
        int n = static_cast<int>(data.size());               // Cache total number of elements in the heap
        while (true) {                                       // Loop until node reaches correct position
            int maxIdx = index;                              // Assume current node is the largest
            int l = left(index);                             // Calculate left child index
            int r = right(index);                            // Calculate right child index

            if (l < n && data[l] > data[maxIdx]) maxIdx = l; // Check if left child is larger
            if (r < n && data[r] > data[maxIdx]) maxIdx = r; // Check if right child is larger

            if (maxIdx == index) break;                      // Heap property satisfied; stop loop

            std::swap(data[index], data[maxIdx]);            // Swap current node with larger child
            index = maxIdx;                                  // Step down into the swapped child's position
        }
    }

public:
    // Default constructor
    MaxHeap() = default;

    // Inserts a new element into the heap
    void insert(T item) {
        data.push_back(std::move(item));                     // Append new element to the end of the vector
        siftUp(static_cast<int>(data.size()) - 1);           // Sift the new element up to its valid position
    }

    // Removes the highest-priority element (root)
    void extractMax() {
        if (data.empty()) return;                            // Guard against calling extract on an empty heap
        data[0] = std::move(data.back());                    // Move the last element to the root position
        data.pop_back();                                     // Remove the last slot from vector
        if (!data.empty()) {                                 // If elements remain, restore heap invariant
            siftDown(0);                                     // Sift the new root element down
        }
    }

    // Returns a const reference to the highest-priority element
    const T& peekMax() const {
        return data[0];                                      // Return root element at index 0
    }

    // Checks whether the heap contains no elements
    bool empty() const { return data.empty(); }

    // Returns current number of elements stored in the heap
    size_t size() const { return data.size(); }
};

// ==========================================
// Class: CEOInboxManager
// ==========================================
// Coordinates commands and prioritizes incoming emails
class CEOInboxManager {
private:
    MaxHeap<Email> inboxQueue;                               // Heap storing prioritized emails
    uint64_t sequenceCounter = 0;                            // Monotonic counter to break ties in order of arrival

    // Trims leading and trailing whitespace from strings
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");     // Locate first non-whitespace character
        if (first == std::string::npos) return "";           // Return empty string if all whitespace
        size_t last = str.find_last_not_of(" \t\r\n");       // Locate last non-whitespace character
        return str.substr(first, (last - first + 1));        // Return substring containing trimmed contents
    }

public:
    // Default constructor
    CEOInboxManager() = default;

    // Instantiates an Email object and inserts it into the heap
    void addEmail(const std::string& senderStr, const std::string& subject, const std::string& date) {
        Category cat = stringToCategory(trim(senderStr));    // Convert parsed sender to enum
        inboxQueue.insert(Email(cat, trim(subject), trim(date), ++sequenceCounter)); // Insert email with incremented ID
    }

    // Handles NEXT command: prints highest priority email without removing it
    void handleNext() const {
        if (inboxQueue.empty()) {                            // Guard against empty inbox
            std::cout << "No emails in inbox.\n";            // Inform user if inbox is empty
            return;                                          // Exit early
        }
        std::cout << "Next email:\n";                        // Output assignment header line
        inboxQueue.peekMax().display();                      // Print top email details
    }

    // Handles READ command: marks highest priority email as read by removing it
    void handleRead() {
        if (inboxQueue.empty()) {                            // Guard against reading from empty inbox
            std::cout << "No emails to read.\n";             // Inform user if no emails remain
            return;                                          // Exit early
        }
        inboxQueue.extractMax();                             // Remove top email from queue
    }

    // Handles COUNT command: displays total unread emails
    void handleCount() const {
        std::cout << "There are " << inboxQueue.size() << " emails to read.\n"; // Print total count
    }

    // Reads and processes commands from a specified test file
    void processCommandFile(const std::string& filename) {
        std::ifstream file(filename);                        // Open input file stream
        if (!file.is_open()) {                               // Check if file failed to open
            std::cerr << "Error: Could not open file " << filename << "\n"; // Print error message
            return;                                          // Abort processing
        }

        std::string line;                                    // String variable to store each file line
        while (std::getline(file, line)) {                   // Read file line by line
            line = trim(line);                               // Strip trailing and leading whitespace
            if (line.empty()) continue;                      // Skip empty lines

            if (line.rfind("EMAIL ", 0) == 0) {              // Check if line starts with command prefix "EMAIL "
                size_t firstComma = line.find(',', 6);       // Find first comma delimiter after "EMAIL " prefix
                size_t secondComma = (firstComma != std::string::npos) ? line.find(',', firstComma + 1) : std::string::npos; // Find second comma

                if (firstComma != std::string::npos && secondComma != std::string::npos) { // Ensure both commas exist
                    std::string sender = line.substr(6, firstComma - 6);                     // Extract sender substring
                    std::string subject = line.substr(firstComma + 1, secondComma - firstComma - 1); // Extract subject substring
                    std::string date = line.substr(secondComma + 1);                         // Extract date substring
                    addEmail(sender, subject, date);                                         // Add parsed email to inbox
                }
            } else if (line == "NEXT") {                     // Check for NEXT command
                handleNext();                                // Delegate to handleNext
            } else if (line == "READ") {                     // Check for READ command
                handleRead();                                // Delegate to handleRead
            } else if (line == "COUNT") {                    // Check for COUNT command
                handleCount();                               // Delegate to handleCount
            }
        }                                                    // End of file processing loop
    }                                                        // File closes automatically upon exiting function scope
};

// ==========================================
// Main Entry Point
// ==========================================
int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);                   // Disable C/C++ I/O stream synchronization for faster speed
    std::cin.tie(nullptr);                                   // Untie cin from cout to prevent unnecessary buffer flushing

    std::string filename = (argc > 1) ? argv[1] : "test.txt";// Use command line argument if supplied, else default to "test.txt"
    CEOInboxManager manager;                                 // Instantiate manager object
    manager.processCommandFile(filename);                    // Process the command file

    return 0;                                                // Return success status code to operating system
}