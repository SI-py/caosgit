#include "utils.h"
#include <functional>
#include <string>
#include <sstream>
#include <iomanip>

std::string simpleHash(const std::string& input) {
    // Очень простой хэш: используем std::hash и возвращаем в hex
    std::hash<std::string> h;
    auto val = h(input);
    std::ostringstream oss;
    oss << std::hex << val;
    return oss.str();
}
