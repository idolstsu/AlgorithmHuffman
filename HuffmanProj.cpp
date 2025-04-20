#include <stdio.h>
#include <iostream>
#include <vector>
#include <queue>
#include <Windows.h>
#include <string>
#include <map>
#include <unordered_map>
#include <fstream>
#include <clocale>
#include <stdlib.h>
#include <bitset>

using namespace std;

// структура для представления узла дерева
struct TreeNode {
    char key;                // символ
    unsigned long long frequency;  // частота символа
    TreeNode* left;          // левый дочерний узел
    TreeNode* right;         // правый дочерний узел
};

// структура для представления дерева хафмана
struct HuffmanTree {
    TreeNode* root;          // корень дерева
};

// структура для сравнения узлов по частоте (для использования в очереди с приоритетом)
struct FrequencyCompare {
    bool operator() (TreeNode* left, TreeNode* right) {
        return left->frequency > right->frequency;  // узел с меньшей частотой должен быть в приоритете
    }
};

// функция для создания нового узла
TreeNode* CreateTreeNode(char ch = '\0', unsigned long long freq = 0, TreeNode* left = NULL, TreeNode* right = NULL) {
    TreeNode* node = new TreeNode();
    node->key = ch;
    node->frequency = freq;
    node->left = left;
    node->right = right;

    return node;
}

// функция для построения дерева хафмана из очереди узлов
HuffmanTree BuildHuffmanTree(priority_queue<TreeNode*, vector<TreeNode*>, FrequencyCompare> nodes) {
    HuffmanTree tree;
    tree.root = NULL;

    // если очередь пуста, возвращаем пустое дерево
    if (nodes.size() == 0)
        return tree;

    // если в очереди только один элемент, он становится корнем
    if (nodes.size() == 1) {
        TreeNode* leaf = nodes.top();
        tree.root = CreateTreeNode('\0', leaf->frequency, leaf, NULL);
        return tree;
    }

    // построение дерева: объединение двух узлов с минимальной частотой
    while (nodes.size() != 1) {
        TreeNode* left = nodes.top();  // извлекаем левый узел
        nodes.pop();
        TreeNode* right = nodes.top();  // извлекаем правый узел
        nodes.pop();

        // создаем новый узел с суммарной частотой и добавляем его обратно в очередь
        nodes.push(CreateTreeNode('\0', left->frequency + right->frequency, left, right));
    }

    // корень дерева,  это оставшийся узел в очереди
    tree.root = nodes.top();
    return tree;
}

// функция для генерации кодов хафмана для каждого символа
void GenerateHuffmanCodes(TreeNode* root, unordered_map<char, string>& huffmanCodes, string code) {
    if (root == NULL) return;

    // если узел — лист, то добавляем его код в таблицу
    if (root->left == NULL && root->right == NULL) {
        if (code == "") {
            huffmanCodes[root->key] = "0";
            return;
        }

        huffmanCodes[root->key] = code;
    }

    // рекурсивный вызов для левого и правого поддерева
    GenerateHuffmanCodes(root->left, huffmanCodes, code + "0");
    GenerateHuffmanCodes(root->right, huffmanCodes, code + "1");
}

// функция для записи числа в бинарном формате в файл
void WriteBinaryNumberToFile(ofstream& out, int number) {
    unsigned char mask = 1;
    unsigned char byte = 0;
    bitset<32> binNumber(number);  // представление числа в бинарном формате
    string binStr = binNumber.to_string();
    int counter = 0;

    // запись каждого бита в байт
    for (int i = 0; i < binStr.length(); i++) {
        byte <<= mask;

        if (binStr[i] == '1') {
            byte |= mask;
        }
        counter++;
        if (counter == 8) {
            counter = 0;
            out.put(byte);
            byte = 0;
        }
    }
}

// функция для чтения числа из файла в бинарном формате
int ReadBinaryNumberFromFile(ifstream& in) {
    unsigned char mask = 1;
    int result = 0;
    string binStr = "";

    // чтение 4 байт из файла и преобразование их в строку бит
    for (int i = 0; i < 4; i++) {
        unsigned char a = in.get();
        bitset<8> b(a);
        binStr += b.to_string();
    }

    // преобразуем строку бит в число
    bitset<32> tmp(binStr);
    return tmp.to_ullong();
}

// функция для записи закодированных данных в файл
void WriteEncodedDataToFile(const string& encode, ofstream& out, map<char, int> Tab) {
    unsigned char byte = 0;
    unsigned char mask = 1;
    size_t counter = 0;

    // временный файл для хранения промежуточных данных
    ofstream tempFile("tmp.txt", ios::binary);

    counter = 0;
    for (int i = 0; i < encode.size(); i++) {
        byte <<= mask;

        if (encode[i] == '1')
            byte |= mask;

        counter++;

        if (counter == 8) {
            tempFile.put(byte);
            byte = 0;
            counter = 0;
        }
    }

    // обработка оставшихся битов
    if (counter > 0) {
        byte <<= (8 - counter);  // дополняем до байта
        tempFile.put(byte);
    }

    tempFile.close();

    // открываем временный файл и записываем закодированные данные в основной файл
    ifstream rtempFile("tmp.txt", ios::binary);
    out << counter;

    // записываем частоты символов в файл
    for (auto p : Tab) {
        if (p.first == '\n') {
            out << '_';
            WriteBinaryNumberToFile(out, p.second);
            continue;
        }
        out << p.first;
        WriteBinaryNumberToFile(out, p.second);
    }
    out << "\n";

    // запись закодированных символов
    char current;
    while (rtempFile.get(current))
        out << current;

    rtempFile.close();
    remove("tmp.txt");
}

// функция для записи дерева Хаффмана в файл
// это нужно для декодирования
void WriteHuffmanTreeToFile(const unordered_map<char, string>& huffmanCodes, ofstream& out) {
    for (auto p : huffmanCodes) {
        if (p.first == '\n') {
            out << "_" << " " << p.second << endl;  // символ перевода строки обрабатывается отдельно
            continue;
        }
        out << p.first << " " << p.second << endl;
    }
}

// декодирование строки с использованием дерева Хаффмана
string DecodeFromTree(TreeNode* root, const string& encode) {
    string decoded = "";
    TreeNode* current = root;

    // проходим по всем битам закодированной строки
    for (int i = 0; i < encode.size(); i++) {
        if (encode[i] == '0')
            current = current->left;
        if (encode[i] == '1')
            current = current->right;

        // когда достигнут лист дерева, добавляем символ в результат
        if (current->left == NULL && current->right == NULL) {
            decoded += current->key;
            current = root;
        }
    }

    return decoded;
}

// декодирование с использованием таблицы кодов Хафмана
string DecodeFromHuffmanCodes(const string& encode, const unordered_map<char, string>& huffmanCodes) {
    string decoded = "";
    string code = "";

    // если в таблице один символ, просто повторяем его
    if (huffmanCodes.size() == 1) {
        for (auto p : huffmanCodes)
            decoded += p.first;
        return decoded;
    }

    // проходим по всем битам закодированной строки
    for (int i = 0; i < encode.size(); i++) {
        code += encode[i];
        for (auto p : huffmanCodes) {
            if (p.second == code) {
                if (p.first == '_') {
                    decoded += '\n';  // обрабатываем символ перевода строки
                    code = "";
                    break;
                }
                decoded += p.first;
                code = "";
                break;
            }
        }
    }

    return decoded;
}

// чтение закодированного текста из файла
string ReadEncodedDataFromFile(ifstream& file, map<char, int>& Tab) {
    string encode = "";
    unsigned char byte;
    unsigned char mask = 1;
    size_t counterBits = file.get() - '0';

    char current;
    while (file.get(current)) {
        if (current == '_') {
            Tab['\n'] = ReadBinaryNumberFromFile(file);
            continue;
        }
        if (current == '\n') {
            break;
        }
        Tab[current] = ReadBinaryNumberFromFile(file);
    }

    // чтение закодированных битов
    while (file.get((char&)byte)) {
        for (int i = 7; i >= 0; i--) {
            unsigned char tmp = (byte >> i);
            if (tmp & mask)
                encode += '1';
            else
                encode += '0';
        }
    }

    encode = encode.substr(0, encode.size() - (8 - counterBits));  // убираем лишние биты
    return encode;
}

void PerformHuffmanCoding(ifstream& in, ofstream& out) {
    string originalText = "";                               // исходный текст
    string encodedText = "";                                 // закодированный текст
    map<char, int> frequencyTable;                           // таблица частот
    priority_queue<TreeNode*, vector<TreeNode*>, FrequencyCompare> queue; // очередь с приоритетом
    unordered_map<char, string> huffmanCodes;                 // таблица кодов хаффмана

    char c;
    while (in.get(c)) {
        originalText += c;
    }

    // строим таблицу частотности символов
    for (int i = 0; i < originalText.size(); i++) {
        frequencyTable[originalText[i]]++;
    }

    // создаем узлы с частотами для каждого символа
    for (auto p : frequencyTable)
        queue.push(CreateTreeNode(p.first, p.second, NULL, NULL));

    // строим дерево хафмана
    HuffmanTree tree = BuildHuffmanTree(queue);

    // формируем таблицу кодов
    GenerateHuffmanCodes(tree.root, huffmanCodes, "");

    // кодируем исходный текст
    for (int i = 0; i < originalText.size(); i++) {
        encodedText += huffmanCodes[originalText[i]];
    }

    size_t bits = 0;
    WriteEncodedDataToFile(encodedText, out, frequencyTable);

    in.close();
    out.close();

    cout << "Compression complete. Encoded data saved to 'encoded.txt'" << endl;
    system("pause");
    return;
}

void PerformDecoding(ifstream& in, ofstream& out) {
    string encodedText = "";
    string decodedText = "";
    unordered_map<char, string> huffmanCodes;
    map<char, int> frequencyTable;
    priority_queue<TreeNode*, vector<TreeNode*>, FrequencyCompare> queue;

    // чтение закодированных данных
    encodedText = ReadEncodedDataFromFile(in, frequencyTable);

    // создание дерева хафмана
    for (auto p : frequencyTable)
        queue.push(CreateTreeNode(p.first, p.second, NULL, NULL));

    HuffmanTree tree = BuildHuffmanTree(queue);

    // генерация таблицы кодов
    GenerateHuffmanCodes(tree.root, huffmanCodes, "");

    // декодируем закодированный текст
    decodedText = DecodeFromHuffmanCodes(encodedText, huffmanCodes);

    // сохраняем декодированный текст в файл
    out << decodedText;

    in.close();
    out.close();

    cout << "Decompression complete. Decoded data saved to 'decoded.txt'" << endl;
    system("pause");
}


int main() {
    // устанавливаем локализацию для работы с русскими символами
    setlocale(LC_ALL, "rus");

    // вводим название файла для сжатия или распаковки
    string filename;
    cout << "Enter the filename to process ( exp.txt to compress | encoded.txt to decompress ): ";
    cin >> filename;

    // открытие входного и выходного файлов
    ifstream in(filename, ios::binary);  // открываем файл для чтения
    if (!in.is_open()) {
        cout << "Error opening input file!" << endl;
        return 1;  // возвращаем ошибку, если файл не открыт
    }

    string option;
    cout << "Enter '1' to compress or '2' to decompress: ";
    cin >> option;

    // если выбрана компрессия
    if (option == "1") {
        ofstream out("encoded.txt", ios::binary);  // открываем файл для записи
        if (!out.is_open()) {
            cout << "Error opening output file for writing!" << endl;
            return 1;
        }

        cout << "Compressing file..." << endl;
        PerformHuffmanCoding(in, out);  // выполняем сжатие с помощью Хаффмана
        cout << "Compression completed successfully!" << endl;
    }
    // если выбрана декомпрессия
    else if (option == "2") {
        ofstream out("decoded.txt", ios::binary);  // открываем файл для записи
        if (!out.is_open()) {
            cout << "Error opening output file for writing!" << endl;
            return 1;
        }

        cout << "Decompressing file..." << endl;
        PerformDecoding(in, out);  // выполняем декомпрессию
        cout << "Decompression completed successfully!" << endl;
    }
    else {
        cout << "Invalid option!" << endl;  // в случае неверного ввода делаем
        return 1;
    }

    in.close();  // закрываем входной файл
    return 0;  // завершаем программу без ошибок
}
