/**
 * @file Attachment.h
 * @brief 附件数据模型
 * 
 * 附件表对应的数据结构，存储消息附件元数据（图片、文件等）。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct Attachment
 * @brief 附件数据结构
 */
struct Attachment {
    uint64_t id;            ///< 主键ID，自增
    uint64_t message_id;    ///< 引用chat_record.id（若为消息附件）
    std::string path;       ///< 存储路径或URL
    std::string mime;       ///< MIME类型
    uint64_t size;          ///< 文件大小（字节）
    int width;              ///< 图片宽度
    int height;             ///< 图片高度
    std::string create_time;///< 创建时间

    /**
     * @brief 构造函数，初始化默认值
     */
    Attachment() : id(0), message_id(0), size(0), width(0), height(0) {}
};
