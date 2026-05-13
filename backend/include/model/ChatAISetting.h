/**
 * @file ChatAISetting.h
 * @brief 聊天AI设置数据模型
 * 
 * 聊天级别的AI配置，与聊天绑定而非用户绑定。
 * 同一聊天中所有成员共享相同的AI设置。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct ChatAISetting
 * @brief 聊天AI设置数据结构
 */
struct ChatAISetting {
    uint64_t id;                ///< 主键ID，自增
    std::string chat_key;       ///< 聊天标识：single:{userId} / group:{groupId} / ai:ai
    std::string nickname;       ///< AI昵称
    int tone;                   ///< 语气：0-默认，1-活泼，2-严谨，3-幽默
    int priority;               ///< 优先级：0-速度优先，1-均衡，2-质量优先
    std::string updated_by;     ///< 最后修改者用户ID
    std::string update_time;    ///< 更新时间

    /**
     * @brief 构造函数，初始化默认值
     */
    ChatAISetting() : id(0), tone(0), priority(1) {}

    /**
     * @brief 获取默认AI设置
     * @return 默认的AI设置对象
     */
    static ChatAISetting getDefault() {
        ChatAISetting setting;
        setting.nickname = "AI助手";
        setting.tone = 0;
        setting.priority = 1;
        return setting;
    }
};
