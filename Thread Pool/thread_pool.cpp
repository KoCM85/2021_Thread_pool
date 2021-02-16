#include "thread_pool.h"

#include <iostream>
#include <ctime>
#include <iomanip>
#include <fstream>

// defines for tasks added to the queue for thread_pool
namespace {
    void pri() {
        std::cout << "pri(): \n";
    }

    void oth() {
        std::cout << "oth() \n";
    }

    int non(int val1, double val2) {
        std::cout << "non() " << val1 << "  -  " << val2 << '\n';

        return val1 + val2;
    }

    class functional_obj {
    public:
        void operator()() {
            std::cout << typeid(*this).name() << '\n';;
        }
    };

    class functa {
    public:
        void operator()(double i, std::string str) {
            std::cout << typeid(*this).name() << "  -  " << i << "  -  " << str << '\n';
        }
    };
}

namespace my {
    void write_error(const std::exception& error_message_in, const std::string& file_name_in) {
        std::ofstream log_file(file_name_in, std::ios_base::out | std::ios_base::app);
        std::time_t time_cur = std::time(nullptr);
        std::tm* time_struct = std::localtime(&time_cur);

        log_file.fill('0');
        log_file << std::setw(2) << time_struct->tm_mday << '.'
            << std::setw(2) << time_struct->tm_mon + 1 << '.'
            << time_struct->tm_year + 1900 << "  "
            << std::setw(2) << time_struct->tm_hour << ':'
            << std::setw(2) << time_struct->tm_min << ':'
            << std::setw(2) << time_struct->tm_sec << " - "
            << error_message_in.what() << '\n';
        
        log_file.close();
    }

    // defines for class random_data
    random_data::random_data() : mt(rand_dev()), distribution_task(min_task, max_task), distribution_val(min_val, max_val) {}

    random_data& random_data::create_instance() {
        static random_data rand_tasks;

        return rand_tasks;
    }

    std::function<void()> random_data::get_task() {
        const short rand_task = distribution_task(mt);

        if (rand_task == 1) {
            return std::function<void()>(pri);
        }
        else if (rand_task == 2) {
            return std::function<void()>(oth);
        }
        else if (rand_task == 3) {
            const unsigned rand_val_1 = distribution_val(mt);
            const unsigned rand_val_2 = distribution_val(mt);

            return std::function<void()>(std::bind(non, rand_val_1, rand_val_2));
        }
        else if (rand_task == 4) {
            return std::function<void()>(functional_obj());
        }
        else if (rand_task == 5) {
            const double rand_val = distribution_val(mt) / rand_task;
            const std::string str = "string for tasks";

            return std::function<void()>(std::bind(functa(), rand_val, str));
        }
    }
}