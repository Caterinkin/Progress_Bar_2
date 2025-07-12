#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <atomic>
#include <sstream>
#include <windows.h>
#include <random>
#include <string>

// Глобальный мьютекс для синхронизации вывода в консоль
std::mutex g_cout_mutex;

// Цвета консоли
enum ConsoleColor {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Yellow = 6,
    White = 7,
    Gray = 8,
    BrightBlue = 9,
    BrightGreen = 10,
    BrightCyan = 11,
    BrightRed = 12,
    BrightMagenta = 13,
    BrightYellow = 14,
    BrightWhite = 15
};

// Установка цвета текста и фона
void SetColor(ConsoleColor text, ConsoleColor background = Black) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (WORD)((background << 4) | text));
}

// Функция для установки позиции курсора
void set_cursor_position(int x, int y) {
    COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Функция для преобразования thread::id в числовое значение
unsigned int thread_id_to_uint(const std::thread::id& id) {
    std::stringstream ss;
    ss << id;
    unsigned int result;
    ss >> result;
    return result;
}

// Класс для управления прогресс-баром потока
class ProgressBar {
private:
    const int thread_num;      // Номер потока по порядку
    const std::thread::id tid; // Идентификатор потока
    const int length;          // Длина прогресс-бара
    int progress = 0;          // Текущий прогресс
    std::vector<bool> errors;  // Отметки об ошибках
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int line_position;         // Позиция строки этого потока
    std::mt19937 gen;          // Генератор случайных чисел

public:
    ProgressBar(int num, std::thread::id id, int len, int pos)
        : thread_num(num), tid(id), length(len), line_position(pos),
        errors(len, false), gen(thread_id_to_uint(id)) {
        start_time = std::chrono::steady_clock::now();
        print_initial();
    }

    // Вывод начальной информации о потоке
    void print_initial() {
        std::lock_guard<std::mutex> lock(g_cout_mutex);
        set_cursor_position(0, line_position);
        SetColor(White);
        std::cout << "Поток " << std::setw(2) << thread_num
            << " (ID: " << thread_id_to_uint(tid) << "): [";
        for (int i = 0; i < length; i++) std::cout << " ";
        std::cout << "]   0%";
        std::cout.flush();
        SetColor(White);
    }

    // Имитация расчета с возможной ошибкой
    void perform_calculation() {
        // 10% вероятность ошибки
        std::uniform_int_distribution<> dis(1, 10);
        if (dis(gen) == 1) { // С вероятностью 1/10
            throw std::runtime_error("Calculation error");
        }

        // Имитация работы
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + gen() % 150));
    }

    // Обновление прогресс-бара
    void update() {
        if (progress >= length) return;

        try {
            perform_calculation();
            progress++;
            errors[progress - 1] = false;
        }
        catch (...) {
            errors[progress] = true;
            progress++;
        }

        std::lock_guard<std::mutex> lock(g_cout_mutex);
        set_cursor_position(18, line_position);

        // Рисуем прогресс-бар
        for (int i = 0; i < length; i++) {
            if (i < progress) {
                if (errors[i]) {
                    SetColor(BrightRed);
                    std::cout << "!";
                }
                else {
                    SetColor(BrightGreen);
                    std::cout << "#";
                }
            }
            else {
                SetColor(White);
                std::cout << " ";
            }
        }

        // Выводим процент выполнения
        SetColor(White);
        std::cout << "] " << std::setw(3) << (100 * progress / length) << "%";
        std::cout.flush();
    }

    // Вывод итоговой информации
    void print_final() {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::lock_guard<std::mutex> lock(g_cout_mutex);
        set_cursor_position(25 + length, line_position);

        // Подсчет ошибок
        int error_count = std::count(errors.begin(), errors.end(), true);
        if (error_count > 0) {
            SetColor(BrightRed);
            std::cout << " Ошибок: " << error_count << " ";
        }

        SetColor(White);
        std::cout << " Время: " << duration.count() << " мс";
        std::cout.flush();
    }

    bool is_complete() const {
        return progress >= length;
    }
};

// Функция, выполняемая в каждом потоке
void calculation_task(int thread_num, int progress_length, int line_pos) {
    ProgressBar pb(thread_num, std::this_thread::get_id(), progress_length, line_pos);

    // Имитация расчета
    while (!pb.is_complete()) {
        pb.update();
    }
    pb.print_final();
}

int main() {
    std::setlocale(LC_ALL, "Russian");

    const int num_threads = 5;      // Количество потоков
    const int progress_length = 30; // Длина прогресс-бара

    // Очищаем консоль и устанавливаем курсор в начало
    system("cls");
    SetColor(White);
    std::cout << "Многопоточный расчет с прогресс-барами (красным отмечены ошибки):\n\n";

    // Создаем вектор потоков
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    // Создаем и запускаем потоки
    for (int i = 1; i <= num_threads; ++i) {
        threads.emplace_back(calculation_task, i, progress_length, 2 + i);
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    // Перемещаем курсор в конец
    set_cursor_position(0, 3 + num_threads);
    std::cout << "\nВсе потоки завершили работу.\n";
    SetColor(White);
    return 0;
}
