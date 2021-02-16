#include "thread_pool.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) try {
    const size_t controller_size = 4;
    using type_func = void();
    my::random_data& random_data1 = my::random_data::create_instance();
    auto lambda_pusher = [](my::controller<type_func, controller_size>& controller_in, my::random_data& random_data_in) {
        while (true) {
            if (controller_in.size() < 100) {
                controller_in.push(random_data_in.get_task());
            }
        }
    };

    my::controller<type_func, controller_size> controller1;
    std::thread controller_pusher(lambda_pusher, std::ref(controller1), std::ref(random_data1));

    controller1.init_threads();
    controller1.listen();

    controller_pusher.join();

    return 0;
}
catch (const std::exception& exc) {
    my::write_error(exc);
}