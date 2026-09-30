#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

// ==========================================
// Class: Email
// Represents an email message with priority logic
// ==========================================
class Email {
private:
    std::string senderCategory;
    std::string subjectLine;
    std::string dateStr; // Format: MM-DD-YYYY

    // Assign numeric ranking to sender categories:
    // Boss (5) > Subordinate (4) > Peer (3) > ImportantPerson (2) > OtherPerson (1)
    int getCategoryRank() const {
        if (senderCategory == "Boss") return 5;
        if (senderCategory == "Subordinate") return 4;
        if (senderCategory == "Peer") return 3;
        if (senderCategory == "ImportantPerson") return 2;
        if (senderCategory == "OtherPerson") return 1;
        return 0;
    }

    // Convert MM-DD-YYYY to an integer YYYYMMDD for chronological comparison
    int getNormalizedDate() const {
        if (dateStr.length() < 10) return 0;
        int month = std::stoi(dateStr.substr(0, 2));
        int day = std::stoi(dateStr.substr(3, 2));
        int year = std::stoi(dateStr.substr(6, 4));
        return year * 10000 + month * 100 + day;
    }

public:
    Email() : senderCategory(""), subjectLine(""), dateStr("") {}

    Email(const std::string& sender, const std::string& subject, const std::string& date)
        : senderCategory(sender), subjectLine(subject), dateStr(date) {}

    const std::string& getSender() const { return senderCategory; }
    const std::string& getSubject() const { return subjectLine; }
    const std::string& getDate() const { return dateStr; }

    // Priority ordering: Higher rank first.
    // If rank is equal, newer date (larger YYYYMMDD) has higher priority.
    bool operator<(const Email& other) const {
        int myRank = getCategoryRank();
        int otherRank = other.getCategoryRank();

        if (myRank != otherRank) {
            return myRank < otherRank;
        }
        return getNormalizedDate() < other.getNormalizedDate();
    }

    bool operator>(const Email& other) const {
        return other < *this;
    }

    void display() const {
        std::cout << "Sender: " << senderCategory << "\n";
        std::cout << "Subject: " << subjectLine << "\n";
        std::cout << "Date: " << dateStr << "\n";
    }
};

// ==========================================
// Class Template: MaxHeap
// List-based implementation created from scratch
// ==========================================
template <typename T>
class MaxHeap {
private:
    std::vector<T> heapList;

    int parent(int index) const { return (index - 1) / 2; }
    int leftChild(int index) const { return 2 * index + 1; }
    int rightChild(int index) const { return 2 * index + 2; }

    void siftUp(int index) {
        while (index > 0 && heapList[index] > heapList[parent(index)]) {
            std::swap(heapList[index], heapList[parent(index)]);
            index = parent(index);
        }
    }

    void siftDown(int index) {
        int maxIndex = index;
        int left = leftChild(index);
        int right = rightChild(index);
        int n = static_cast<int>(heapList.size());

        if (left < n && heapList[left] > heapList[maxIndex]) {
            maxIndex = left;
        }

        if (right < n && heapList[right] > heapList[maxIndex]) {
            maxIndex = right;
        }

        if (index != maxIndex) {
            std::swap(heapList[index], heapList[maxIndex]);
            siftDown(maxIndex);
        }
    }

public:
    MaxHeap() = default;

    void insert(const T& item) {
        heapList.push_back(item);
        siftUp(static_cast<int>(heapList.size()) - 1);
    }

    T extractMax() {
        if (heapList.empty()) {
            throw std::underflow_error("Heap is empty.");
        }
        T maxValue = heapList[0];
        heapList[0] = heapList.back();
        heapList.pop_back();

        if (!heapList.empty()) {
            siftDown(0);
        }
        return maxValue;
    }

    const T& peekMax() const {
        if (heapList.empty()) {
            throw std::underflow_error("Heap is empty.");
        }
        return heapList[0];
    }

    bool isEmpty() const {
        return heapList.empty();
    }

    int size() const {
        return static_cast<int>(heapList.size());
    }
};

// ==========================================
// Class: CEOInboxManager
// Orchestrates commands and queue handling
// ==========================================
class CEOInboxManager {
private:
    MaxHeap<Email> inboxQueue;

    // Helper method to trim leading and trailing whitespace
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    CEOInboxManager() = default;

    void addEmail(const std::string& sender, const std::string& subject, const std::string& date) {
        Email email(trim(sender), trim(subject), trim(date));
        inboxQueue.insert(email);
    }

    void handleNext() const {
        if (inboxQueue.isEmpty()) {
            std::cout << "No emails in inbox.\n";
            return;
        }
        std::cout << "Next email:\n";
        inboxQueue.peekMax().display();
    }

    void handleRead() {
        if (inboxQueue.isEmpty()) {
            std::cout << "No emails to read.\n";
            return;
        }
        inboxQueue.extractMax();
    }

    void handleCount() const {
        std::cout << "There are " << inboxQueue.size() << " emails to read.\n";
    }

    void processCommandFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << "\n";
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;

            if (line.rfind("EMAIL ", 0) == 0) {
                // Remove the "EMAIL " command prefix
                std::string payload = line.substr(6);
                std::stringstream ss(payload);
                std::string sender, subject, date;

                if (std::getline(ss, sender, ',') &&
                    std::getline(ss, subject, ',') &&
                    std::getline(ss, date)) {
                    addEmail(sender, subject, date);
                }
            } else if (line == "NEXT") {
                handleNext();
            } else if (line == "READ") {
                handleRead();
            } else if (line == "COUNT") {
                handleCount();
            }
        }
        file.close();
    }
};

// ==========================================
// Main Entry Point
// ==========================================
int main(int argc, char* argv[]) {
    std::string filename = "test.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    CEOInboxManager manager;
    manager.processCommandFile(filename);

    return 0;
}