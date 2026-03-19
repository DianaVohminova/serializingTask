/*
    g++ -o Serializer main.cpp
    ./Serializer
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>



// Структура узла списка
struct ListNode {
    ListNode* prev = nullptr; // указатель на предыдущий элемент или nullptr
    ListNode* next = nullptr; // указатель на следующий элемент или nullptr
    ListNode* rand = nullptr; // указатель на произвольный элемент данного списка либо nullptr
    std::string data;         // произвольные пользовательские данные
};

enum class FileResult {
    Success,
    CannotOpenInput,
    CannotCreateOutput,
    FailedToRead,
    FailedToWrite,
    InvalidFormat // ошибки формата
};

class List {
public:
    List();
    ~List();

    FileResult readFromFile(const std::string& filename, std::string& error_message);
    FileResult serialize(std::ostream& stream, std::string& error_message);  // объявление метода сериализации
    FileResult deserialize(std::istream& stream, std::string& error_message);    // объявление метода десериализации
    FileResult serializeToFile(const std::string& filename, std::string& error_message);
    FileResult deserializeFromFile(const std::string& filename, std::string& error_message);

    // --- Дополнительные методы для отладки ---
    ListNode* GetHead() const; // Объявление константного метода
    int GetCount() const;      // Объявление константного метода

private:
    ListNode* head;
    ListNode* tail;
    int count;

    void clear(); // функция очистки
};

// реализация методов List

List::List() : head(nullptr), tail(nullptr), count(0) {}

List::~List() {
    clear();
}

FileResult List::readFromFile(const std::string& filename, std::string& error_message) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        error_message = "Cannot open input file: " + filename;
        return FileResult::CannotOpenInput;
    }

    clear(); // Очищаем текущий список перед загрузкой нового

    std::string line;
    std::vector<std::pair<std::string, int>> parsed_data;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        size_t delimiter_pos = line.rfind(';');
        if (delimiter_pos == std::string::npos) {
            parsed_data.emplace_back(line, -1);
            continue;
        }

        std::string data_str = line.substr(0, delimiter_pos);
        std::string index_str = line.substr(delimiter_pos + 1);

        int rand_index;
        try {
            rand_index = std::stoi(index_str);
        } catch (...) {
            // Если индекс не распознан, считаем его -1 (nullptr)
            rand_index = -1;
            file.close();
        }

        parsed_data.emplace_back(std::move(data_str), rand_index);
    }
    file.close();

    int num_nodes = parsed_data.size();
    if (num_nodes == 0) {
        return FileResult::Success; // Пустой список - валидный результат
    }

    // Создаем узлы и связываем их через указатели prev/next
    std::vector<ListNode*> nodes;
    nodes.reserve(num_nodes);

    for (int i = 0; i < num_nodes; ++i) {
        ListNode* new_node = new ListNode();
        new_node->data = std::move(parsed_data[i].first);
        nodes.push_back(new_node);

        if (i > 0) {
            new_node->prev = nodes[i - 1];
            nodes[i - 1]->next = new_node;
        }
    }

    // устанавливаем голову и хвост списка
    head = nodes[0];
    tail = nodes[num_nodes - 1];
    count = num_nodes;

    // Устанавливаем rand-ссылки
    for (int i = 0; i < num_nodes; ++i) {
        int target_idx = parsed_data[i].second;
        if (target_idx >= 0 && target_idx < num_nodes) {
            nodes[i]->rand = nodes[target_idx];
        } else if(target_idx != -1) { // Если индекс не -1 и не в диапазоне [0, count), это ошибка
             error_message = "Rand index out of bounds in node " + std::to_string(i) + ", index: " + std::to_string(target_idx);
             clear(); // Очищаем частично построенный список
             return FileResult::InvalidFormat;
        } else {
            nodes[i]->rand = nullptr;
        }
    }
    return FileResult::Success;
}
/*--------------------------------------*/
// функция сериализации
FileResult List::serialize(std::ostream& stream, std::string& error_message) {
    stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
    if (stream.fail()) {
        error_message = "Failed to write count.";
        return FileResult::FailedToWrite;
    }

    if (count == 0) { // пустой список
        return FileResult::Success;
    }

    // нумеруем узлы и собираем их в вектор для быстрого доступа по индексу
    std::vector<ListNode*> node_vec;
    node_vec.reserve(count);
    ListNode* current = head;
    while (current != nullptr) {
        node_vec.push_back(current);
        current = current->next;
    }

    // записываем длины  и содержимое строк data
    for (int i = 0; i < count; ++i) {
        const std::string& data_str = node_vec[i]->data;
        int data_len = static_cast<int>(data_str.length());
        stream.write(reinterpret_cast<const char*>(&data_len), sizeof(data_len));
        if (stream.fail()) {
            error_message = "Failed to write string length.";
            return FileResult::FailedToWrite;
        }
        stream.write(data_str.data(), data_len);
        if (stream.fail()) {
            error_message = "Failed to write string data.";
            return FileResult::FailedToWrite;
        }
    }

    // записываем индексы rand-ссылок
    for (int i = 0; i < count; ++i) {
        ListNode* rand_ptr = node_vec[i]->rand;
        int rand_idx = -1;

        if (rand_ptr != nullptr) {
            for (int j = 0; j < count; ++j) {
                if (node_vec[j] == rand_ptr) {
                    rand_idx = j;
                    break;
                }
            }
        }
        stream.write(reinterpret_cast<const char*>(&rand_idx), sizeof(rand_idx));
        if (stream.fail()) {
            error_message = "Failed to write rand index.";
            return FileResult::FailedToWrite;
        }
    }
    return FileResult::Success;
}

   


int main() {
    List list;
    std::string error_msg; // Объявляем переменную для сообщения об ошибке

    // Выполняем операции и проверяем результат
    FileResult result = list.readFromFile("inlet.in", error_msg);

    if (result != FileResult::Success) {
        std::cerr << "Error reading file: " << error_msg << std::endl;
        return 1; // Завершаем с ошибкой
    }

    result = list.serializeToFile("outlet.out", error_msg);

    if (result != FileResult::Success) {
        std::cerr << "Error serializing file: " << error_msg << std::endl;
        return 1; // Завершаем с ошибкой
    }

    return 0; // Успешное завершение
}