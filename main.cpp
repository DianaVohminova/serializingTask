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

/*---------------------------------------------------------*/
// Функция десериализации 
FileResult List::deserialize(std::istream& stream, std::string& error_message) {
    clear();

    int num_nodes;
    stream.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));
    if (stream.fail()) {
        error_message = "Failed to read count during deserialization.";
        return FileResult::FailedToRead;
    }

    if (num_nodes <= 0) {
        count = 0;
        head = nullptr;
        tail = nullptr;
        return FileResult::Success; // Считаем пустой список за успех
    }

    // Проверим лимит по ТЗ (10^6)
    if (num_nodes > 1000000) {
        error_message = "Number of nodes exceeds limit (" + std::to_string(num_nodes) + " > 1000000)";
        return FileResult::InvalidFormat;
    }

    std::vector<ListNode*> nodes;
    nodes.reserve(num_nodes);

    for (int i = 0; i < num_nodes; ++i) {
        int data_len;
        stream.read(reinterpret_cast<char*>(&data_len), sizeof(data_len));
        if (stream.fail()) {
            error_message = "Failed to read string length during deserialization.";
            return FileResult::FailedToRead;
        }

        if (data_len < 0 || data_len > 1000) { // Проверим лимит длины строки из ТЗ
            error_message = "Invalid string length found during deserialization: " + std::to_string(data_len);
            return FileResult::InvalidFormat;
        }

        std::string data_str(data_len, '\0');
        stream.read(&data_str[0], data_len);
        if (stream.fail()) {
            error_message = "Failed to read string data during deserialization.";
            return FileResult::FailedToRead;
        }

        ListNode* new_node = new ListNode();
        new_node->data = std::move(data_str);
        nodes.push_back(new_node);
    }

    for (int i = 0; i < num_nodes; ++i) {
        if (i > 0) {
            nodes[i]->prev = nodes[i - 1];
        }
        if (i < num_nodes - 1) {
            nodes[i]->next = nodes[i + 1];
        }
    }

    head = nodes[0];
    tail = nodes[num_nodes - 1];
    count = num_nodes;

    for (int i = 0; i < num_nodes; ++i) {
        int rand_idx;
        stream.read(reinterpret_cast<char*>(&rand_idx), sizeof(rand_idx));
        if (stream.fail()) {
            error_message = "Failed to read rand index during deserialization.";
            return FileResult::FailedToRead;
        }

        if (rand_idx >= 0 && rand_idx < num_nodes) {
            nodes[i]->rand = nodes[rand_idx];
        } else if (rand_idx != -1) {
            error_message = "Rand index out of bounds in serialized data for node " + std::to_string(i) + ", index: " + std::to_string(rand_idx);
            clear();
            return FileResult::InvalidFormat;
        } else {
            nodes[i]->rand = nullptr;
        }
    }
    return FileResult::Success;
}
/*-----------------------------------------------------------*/
// Запись сериализованных данных в файл outlet.out
FileResult List::serializeToFile(const std::string& filename, std::string& error_message) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        error_message = "Cannot create output file: " + filename;
        return FileResult::CannotCreateOutput;
    }
    FileResult res = serialize(file, error_message);
    file.close();
    return res; // Возвращаем результат Serialize
}
/*-----------------------------------------------------------*/
// Считывание сериализованных данных из файла outlet.out и десериализация
FileResult List::deserializeFromFile(const std::string& filename, std::string& error_message) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        error_message = "Cannot open binary file: " + filename;
        return FileResult::CannotOpenInput;
    }
    FileResult res = deserialize(file, error_message);
    file.close();
    return res; // Возвращаем результат Deserialize
}
/*-----------------------------------------------------------*/
// Вспомогательные функции для отладки
ListNode* List::GetHead() const {
    return head;
}

int List::GetCount() const {
    return count;
}

/*-----------------------------------------------------------*/
// Функция очистки списка
void List::clear() {
    ListNode* current = head;
    while (current != nullptr) {
        ListNode* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
    tail = nullptr;
    count = 0;
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

    // Десериализация
    List listDes;
    result = listDes.deserializeFromFile("outlet.out", error_msg);
    if (result != FileResult::Success) {
        std::cerr << "Error deserializing file: " << error_msg << std::endl;
        return 1;
    }

    // Вывод на экран десериализованных данных
    ListNode* current = listDes.GetHead();
        int i = 0;
        while (current != nullptr) {
            std::cout << "Node " << i << ": Data= " << current->data << "\", Rand=\"";
            if (current->rand) {
                // Простая проверка: найдем индекс rand узла
                 ListNode* temp = listDes.GetHead();
                 int idx = 0;
                 while(temp && temp != current->rand) { temp = temp->next; ++idx; }
                 std::cout << "Node " << idx << "(Data:" << current->rand->data << ")";
            } else {
                std::cout << "nullptr";
            }
            std::cout << std::endl;
            current = current->next;
            ++i;
        }
    std::cout << "Deserialized list count: " << listDes.GetCount() << std::endl;

    return 0; // Успешное завершение
}