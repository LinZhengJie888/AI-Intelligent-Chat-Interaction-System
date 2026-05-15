/**
 * @file JsonUtil.cpp
 * @brief JSON解析工具类实现
 */

#include "common/JsonUtil.h"
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

JsonValue JsonUtil::parse(const std::string& json) {
    size_t pos = 0;
    skipWhitespace(json, pos);
    return parseValue(json, pos);
}

std::string JsonUtil::stringify(const JsonValue& value) {
    std::ostringstream oss;
    
    switch (value.getType()) {
        case JsonValue::NULL_TYPE:
            oss << "null";
            break;
        case JsonValue::BOOL_TYPE:
            oss << (value.toBool() ? "true" : "false");
            break;
        case JsonValue::INT_TYPE:
            oss << value.toInt();
            break;
        case JsonValue::DOUBLE_TYPE:
            oss << value.toDouble();
            break;
        case JsonValue::STRING_TYPE:
            oss << "\"" << escapeString(value.toString()) << "\"";
            break;
        case JsonValue::ARRAY_TYPE: {
            oss << "[";
            const auto& arr = value.toArray();
            for (size_t i = 0; i < arr.size(); i++) {
                if (i > 0) oss << ",";
                oss << stringify(arr[i]);
            }
            oss << "]";
            break;
        }
        case JsonValue::OBJECT_TYPE: {
            oss << "{";
            const auto& obj = value.toObject();
            bool first = true;
            for (const auto& pair : obj) {
                if (!first) oss << ",";
                first = false;
                oss << "\"" << escapeString(pair.first) << "\":" << stringify(pair.second);
            }
            oss << "}";
            break;
        }
    }
    
    return oss.str();
}

std::string JsonUtil::getString(const std::string& json, const std::string& key) {
    try {
        JsonValue value = parse(json);
        if (value.isObject() && value.hasKey(key)) {
            const JsonValue& field = value[key];
            if (field.isString()) {
                return field.toString();
            } else if (field.isArray() || field.isObject()) {
                // 对于数组和对象，返回序列化的 JSON 字符串
                return stringify(field);
            }
        }
    } catch (...) {
        // 解析失败，回退到简单查找
    }
    
    // 回退到简单查找（兼容旧代码）
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    pos++;
    
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.size()) return "";
    
    std::string value;
    if (json[pos] == '"') {
        pos++;
        size_t end = pos;
        while (end < json.size()) {
            if (json[end] == '\\' && end + 1 < json.size()) {
                end += 2;
                continue;
            }
            if (json[end] == '"') break;
            end++;
        }
        if (end >= json.size()) return "";
        value = json.substr(pos, end - pos);
    } else {
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) end = json.size();
        value = json.substr(pos, end - pos);
    }
    
    return value;
}

int JsonUtil::getInt(const std::string& json, const std::string& key, int default_value) {
    try {
        JsonValue value = parse(json);
        if (value.isObject() && value.hasKey(key)) {
            const JsonValue& field = value[key];
            if (field.isInt()) {
                return field.toInt();
            }
        }
    } catch (...) {}
    return default_value;
}

bool JsonUtil::getBool(const std::string& json, const std::string& key, bool default_value) {
    try {
        JsonValue value = parse(json);
        if (value.isObject() && value.hasKey(key)) {
            const JsonValue& field = value[key];
            if (field.isBool()) {
                return field.toBool();
            }
        }
    } catch (...) {}
    return default_value;
}

JsonValue JsonUtil::parseValue(const std::string& json, size_t& pos) {
    skipWhitespace(json, pos);
    
    if (pos >= json.size()) {
        return JsonValue();
    }
    
    char c = json[pos];
    
    if (c == '"') {
        return JsonValue(parseString(json, pos));
    } else if (c == '{') {
        return parseObject(json, pos);
    } else if (c == '[') {
        return parseArray(json, pos);
    } else if (c == 't' || c == 'f') {
        // 布尔值
        if (json.substr(pos, 4) == "true") {
            pos += 4;
            return JsonValue(true);
        } else if (json.substr(pos, 5) == "false") {
            pos += 5;
            return JsonValue(false);
        }
    } else if (c == 'n') {
        // null
        if (json.substr(pos, 4) == "null") {
            pos += 4;
            return JsonValue();
        }
    } else if (c == '-' || std::isdigit(c)) {
        return parseNumber(json, pos);
    }
    
    return JsonValue();
}

JsonValue JsonUtil::parseObject(const std::string& json, size_t& pos) {
    std::map<std::string, JsonValue> result;
    
    pos++; // 跳过 '{'
    skipWhitespace(json, pos);
    
    if (pos < json.size() && json[pos] == '}') {
        pos++;
        return JsonValue(result);
    }
    
    while (pos < json.size()) {
        skipWhitespace(json, pos);
        
        // 解析键
        if (pos >= json.size() || json[pos] != '"') {
            break;
        }
        std::string key = parseString(json, pos);
        
        // 跳过 ':'
        skipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            break;
        }
        pos++;
        
        // 解析值
        skipWhitespace(json, pos);
        JsonValue value = parseValue(json, pos);
        
        result[key] = value;
        
        // 检查逗号或结束
        skipWhitespace(json, pos);
        if (pos >= json.size()) break;
        
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] == '}') {
            pos++;
            return JsonValue(result);
        } else {
            break;
        }
    }
    
    return JsonValue(result);
}

JsonValue JsonUtil::parseArray(const std::string& json, size_t& pos) {
    std::vector<JsonValue> result;
    
    pos++; // 跳过 '['
    skipWhitespace(json, pos);
    
    if (pos < json.size() && json[pos] == ']') {
        pos++;
        return JsonValue(result);
    }
    
    while (pos < json.size()) {
        skipWhitespace(json, pos);
        JsonValue value = parseValue(json, pos);
        result.push_back(value);
        
        skipWhitespace(json, pos);
        if (pos >= json.size()) break;
        
        if (json[pos] == ',') {
            pos++;
        } else if (json[pos] == ']') {
            pos++;
            return JsonValue(result);
        } else {
            break;
        }
    }
    
    return JsonValue(result);
}

std::string JsonUtil::parseString(const std::string& json, size_t& pos) {
    std::string result;
    
    pos++; // 跳过开头的 '"'
    
    while (pos < json.size()) {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            pos++;
            switch (json[pos]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    // Unicode转义
                    if (pos + 4 < json.size()) {
                        std::string hex = json.substr(pos + 1, 4);
                        unsigned long codepoint = std::stoul(hex, nullptr, 16);
                        
                        if (codepoint <= 0x7F) {
                            result += static_cast<char>(codepoint);
                        } else if (codepoint <= 0x7FF) {
                            result += static_cast<char>(0xC0 | (codepoint >> 6));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (codepoint >> 12));
                            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }
                        pos += 4;
                    }
                    break;
                }
                default: result += json[pos]; break;
            }
            pos++;
        } else if (json[pos] == '"') {
            pos++;
            return result;
        } else {
            result += json[pos];
            pos++;
        }
    }
    
    return result;
}

JsonValue JsonUtil::parseNumber(const std::string& json, size_t& pos) {
    size_t start = pos;
    bool is_float = false;
    
    if (pos < json.size() && json[pos] == '-') {
        pos++;
    }
    
    while (pos < json.size() && std::isdigit(json[pos])) {
        pos++;
    }
    
    if (pos < json.size() && json[pos] == '.') {
        is_float = true;
        pos++;
        while (pos < json.size() && std::isdigit(json[pos])) {
            pos++;
        }
    }
    
    if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
        is_float = true;
        pos++;
        if (pos < json.size() && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        while (pos < json.size() && std::isdigit(json[pos])) {
            pos++;
        }
    }
    
    std::string num_str = json.substr(start, pos - start);
    
    if (is_float) {
        return JsonValue(std::stod(num_str));
    } else {
        return JsonValue(std::stoi(num_str));
    }
}

void JsonUtil::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && std::isspace(json[pos])) {
        pos++;
    }
}

std::string JsonUtil::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}
