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
    Success,            // Успешно
    CannotOpenInput,    // Ошибка открытия файла
    CannotCreateOutput,
    FaliedToRead,
    FailedToWrite
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
    int count(); // Объявление приватной функции
};

// реализация методов List

List::List() : head(nullptr), tail(nullptr), count(0) {}

List::~List() {
    clear();
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