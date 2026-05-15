/**
 * @file JsonUtil.h
 * @brief JSON解析工具类
 * 
 * 提供健壮的JSON解析功能，支持嵌套对象、数组、转义字符等。
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <variant>

/**
 * @class JsonValue
 * @brief JSON值类型
 */
class JsonValue {
public:
    enum Type {
        NULL_TYPE,
        BOOL_TYPE,
        INT_TYPE,
        DOUBLE_TYPE,
        STRING_TYPE,
        ARRAY_TYPE,
        OBJECT_TYPE
    };
    
    JsonValue() : type_(NULL_TYPE) {}
    JsonValue(bool value) : type_(BOOL_TYPE), bool_value_(value) {}
    JsonValue(int value) : type_(INT_TYPE), int_value_(value) {}
    JsonValue(double value) : type_(DOUBLE_TYPE), double_value_(value) {}
    JsonValue(const std::string& value) : type_(STRING_TYPE), string_value_(value) {}
    JsonValue(const char* value) : type_(STRING_TYPE), string_value_(value) {}
    JsonValue(const std::vector<JsonValue>& value) : type_(ARRAY_TYPE), array_value_(value) {}
    JsonValue(const std::map<std::string, JsonValue>& value) : type_(OBJECT_TYPE), object_value_(value) {}
    
    Type getType() const { return type_; }
    bool isNull() const { return type_ == NULL_TYPE; }
    bool isBool() const { return type_ == BOOL_TYPE; }
    bool isInt() const { return type_ == INT_TYPE; }
    bool isDouble() const { return type_ == DOUBLE_TYPE; }
    bool isString() const { return type_ == STRING_TYPE; }
    bool isArray() const { return type_ == ARRAY_TYPE; }
    bool isObject() const { return type_ == OBJECT_TYPE; }
    
    bool toBool() const { return bool_value_; }
    int toInt() const { return int_value_; }
    double toDouble() const { return double_value_; }
    const std::string& toString() const { return string_value_; }
    const std::vector<JsonValue>& toArray() const { return array_value_; }
    const std::map<std::string, JsonValue>& toObject() const { return object_value_; }
    
    // 访问对象成员
    bool hasKey(const std::string& key) const {
        return type_ == OBJECT_TYPE && object_value_.find(key) != object_value_.end();
    }
    
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_value;
        if (type_ != OBJECT_TYPE) return null_value;
        auto it = object_value_.find(key);
        if (it == object_value_.end()) return null_value;
        return it->second;
    }
    
    // 访问数组元素
    size_t size() const {
        if (type_ == ARRAY_TYPE) return array_value_.size();
        if (type_ == OBJECT_TYPE) return object_value_.size();
        return 0;
    }
    
    const JsonValue& operator[](size_t index) const {
        static JsonValue null_value;
        if (type_ != ARRAY_TYPE || index >= array_value_.size()) return null_value;
        return array_value_[index];
    }

private:
    Type type_;
    bool bool_value_ = false;
    int int_value_ = 0;
    double double_value_ = 0.0;
    std::string string_value_;
    std::vector<JsonValue> array_value_;
    std::map<std::string, JsonValue> object_value_;
};

/**
 * @class JsonUtil
 * @brief JSON解析工具类
 */
class JsonUtil {
public:
    /**
     * @brief 解析JSON字符串
     * @param json JSON字符串
     * @return 解析后的JsonValue
     */
    static JsonValue parse(const std::string& json);
    
    /**
     * @brief 将JsonValue序列化为JSON字符串
     * @param value JsonValue
     * @return JSON字符串
     */
    static std::string stringify(const JsonValue& value);
    
    /**
     * @brief 从JSON字符串中获取指定键的值（兼容旧接口）
     * @param json JSON字符串
     * @param key 键名
     * @return 键值字符串
     */
    static std::string getString(const std::string& json, const std::string& key);
    
    /**
     * @brief 从JSON字符串中获取指定键的整数值
     * @param json JSON字符串
     * @param key 键名
     * @param default_value 默认值
     * @return 整数值
     */
    static int getInt(const std::string& json, const std::string& key, int default_value = 0);
    
    /**
     * @brief 从JSON字符串中获取指定键的布尔值
     * @param json JSON字符串
     * @param key 键名
     * @param default_value 默认值
     * @return 布尔值
     */
    static bool getBool(const std::string& json, const std::string& key, bool default_value = false);
    
    /**
     * @brief 转义JSON字符串
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    static std::string escapeString(const std::string& str);

private:
    /**
     * @brief 解析JSON值
     * @param json JSON字符串
     * @param pos 当前位置
     * @return 解析后的JsonValue
     */
    static JsonValue parseValue(const std::string& json, size_t& pos);
    
    /**
     * @brief 解析JSON对象
     * @param json JSON字符串
     * @param pos 当前位置
     * @return 解析后的JsonValue
     */
    static JsonValue parseObject(const std::string& json, size_t& pos);
    
    /**
     * @brief 解析JSON数组
     * @param json JSON字符串
     * @param pos 当前位置
     * @return 解析后的JsonValue
     */
    static JsonValue parseArray(const std::string& json, size_t& pos);
    
    /**
     * @brief 解析JSON字符串
     * @param json JSON字符串
     * @param pos 当前位置
     * @return 解析后的字符串
     */
    static std::string parseString(const std::string& json, size_t& pos);
    
    /**
     * @brief 解析JSON数字
     * @param json JSON字符串
     * @param pos 当前位置
     * @return 解析后的JsonValue
     */
    static JsonValue parseNumber(const std::string& json, size_t& pos);
    
    /**
     * @brief 跳过空白字符
     * @param json JSON字符串
     * @param pos 当前位置
     */
    static void skipWhitespace(const std::string& json, size_t& pos);
};
