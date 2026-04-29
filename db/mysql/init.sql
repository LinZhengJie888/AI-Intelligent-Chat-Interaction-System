-- ============================================
-- AI智能聊天互动系统 - MySQL数据库初始化脚本
-- ============================================

-- 创建数据库
CREATE DATABASE IF NOT EXISTS ai_chat_system DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE ai_chat_system;

-- ============================================
-- 1. 用户表 (user)
-- ============================================
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `user_id` VARCHAR(64) NOT NULL COMMENT '用户ID（唯一标识，不可修改）',
    `username` VARCHAR(64) NOT NULL COMMENT '用户名（可重复，可修改）',
    `nickname` VARCHAR(64) DEFAULT NULL COMMENT '昵称',
    `password` VARCHAR(255) NOT NULL COMMENT '加密密码',
    `phone` VARCHAR(20) DEFAULT NULL COMMENT '手机号',
    `avatar_path` VARCHAR(255) DEFAULT NULL COMMENT '用户头像存储路径',
    `ai_nickname` VARCHAR(64) DEFAULT 'AI助手' COMMENT 'AI助手昵称',
    `ai_tone` TINYINT DEFAULT 0 COMMENT 'AI语气（0-活泼，1-严谨，2-幽默）',
    `ai_priority` TINYINT DEFAULT 0 COMMENT 'AI响应优先级（0-速度优先，1-质量优先）',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_user_id` (`user_id`),
    KEY `idx_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

-- ============================================
-- 2. 群聊表 (group_chat)
-- ============================================
DROP TABLE IF EXISTS `group_chat`;
CREATE TABLE `group_chat` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `group_id` VARCHAR(64) NOT NULL COMMENT '群聊专属ID（唯一标识）',
    `group_name` VARCHAR(64) NOT NULL COMMENT '群名（可修改）',
    `creator_id` BIGINT UNSIGNED NOT NULL COMMENT '创建者用户ID',
    `avatar_path` VARCHAR(255) DEFAULT NULL COMMENT '群聊头像存储路径',
    `announcement` TEXT DEFAULT NULL COMMENT '群公告',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_group_id` (`group_id`),
    KEY `idx_creator_id` (`creator_id`),
    KEY `idx_group_name` (`group_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='群聊表';

-- ============================================
-- 3. 群成员表 (group_member)
-- ============================================
DROP TABLE IF EXISTS `group_member`;
CREATE TABLE `group_member` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `group_id` BIGINT UNSIGNED NOT NULL COMMENT '群聊ID',
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    `role` TINYINT DEFAULT 0 COMMENT '角色（0-普通成员，1-管理员，2-群主）',
    `nickname_in_group` VARCHAR(64) DEFAULT NULL COMMENT '群内昵称',
    `mute_until` DATETIME DEFAULT NULL COMMENT '禁言至',
    `join_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_group_user` (`group_id`, `user_id`),
    KEY `idx_user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='群成员表';

-- ============================================
-- 4. 好友关系表 (friend_relation)
-- ============================================
DROP TABLE IF EXISTS `friend_relation`;
CREATE TABLE `friend_relation` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `user_id` BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    `friend_id` BIGINT UNSIGNED NOT NULL COMMENT '好友用户ID',
    `remark` VARCHAR(64) DEFAULT NULL COMMENT '备注名',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_user_friend` (`user_id`, `friend_id`),
    KEY `idx_friend_id` (`friend_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='好友关系表';

-- ============================================
-- 5. 好友请求表 (friend_request)
-- ============================================
DROP TABLE IF EXISTS `friend_request`;
CREATE TABLE `friend_request` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `from_user_id` BIGINT UNSIGNED NOT NULL COMMENT '请求方用户ID',
    `to_user_id` BIGINT UNSIGNED NOT NULL COMMENT '接收方用户ID',
    `request_msg` VARCHAR(255) DEFAULT NULL COMMENT '验证消息',
    `status` TINYINT DEFAULT 0 COMMENT '状态（0-待处理，1-同意，2-拒绝）',
    `cooling_time` DATETIME DEFAULT NULL COMMENT '冷却时间',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_from_user` (`from_user_id`),
    KEY `idx_to_user` (`to_user_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='好友请求表';

-- ============================================
-- 6. 加群请求表 (group_request)
-- ============================================
DROP TABLE IF EXISTS `group_request`;
CREATE TABLE `group_request` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `group_id` BIGINT UNSIGNED NOT NULL COMMENT '群聊ID',
    `from_user_id` BIGINT UNSIGNED NOT NULL COMMENT '申请人用户ID',
    `request_msg` VARCHAR(255) DEFAULT NULL COMMENT '申请消息',
    `status` TINYINT DEFAULT 0 COMMENT '状态（0-待处理，1-同意，2-拒绝）',
    `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (`id`),
    KEY `idx_group_id` (`group_id`),
    KEY `idx_from_user` (`from_user_id`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='加群请求表';

-- ============================================
-- 7. 聊天记录表 (chat_record)
-- ============================================
DROP TABLE IF EXISTS `chat_record`;
CREATE TABLE `chat_record` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `sender_id` BIGINT UNSIGNED NOT NULL COMMENT '发送者用户ID',
    `receiver_id` BIGINT UNSIGNED DEFAULT NULL COMMENT '接收者用户ID（一对一聊天时使用）',
    `group_id` BIGINT UNSIGNED DEFAULT NULL COMMENT '群聊ID（群聊时使用）',
    `content` TEXT NOT NULL COMMENT '消息内容',
    `msg_type` TINYINT DEFAULT 0 COMMENT '消息类型（0-文字，1-图片，2-表情）',
    `is_ai` TINYINT DEFAULT 0 COMMENT '是否为AI回复（0-否，1-是）',
    `is_recalled` TINYINT DEFAULT 0 COMMENT '是否已撤回（0-否，1-是）',
    `is_read` TINYINT DEFAULT 0 COMMENT '是否已读（0-未读，1-已读）',
    `send_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    PRIMARY KEY (`id`),
    KEY `idx_sender_id` (`sender_id`),
    KEY `idx_receiver_id` (`receiver_id`),
    KEY `idx_group_id` (`group_id`),
    KEY `idx_send_time` (`send_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='聊天记录表';

-- ============================================
-- 8. 验证码表 (verification_code)
-- ============================================
DROP TABLE IF EXISTS `verification_code`;
CREATE TABLE `verification_code` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
    `phone` VARCHAR(20) NOT NULL COMMENT '手机号',
    `code` VARCHAR(16) NOT NULL COMMENT '验证码（6位字符组合）',
    `captcha_token` VARCHAR(64) DEFAULT NULL COMMENT '图像验证码关联 token（用于前端刷新/校验）',
    `expire_time` DATETIME NOT NULL COMMENT '过期时间',
    `send_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    `is_used` TINYINT DEFAULT 0 COMMENT '是否已使用（0-未使用，1-已使用）',
    PRIMARY KEY (`id`),
    KEY `idx_phone` (`phone`),
    KEY `idx_expire_time` (`expire_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='验证码表';

-- ============================================
-- 初始化完成提示
-- ============================================
SELECT '数据库初始化完成！' AS message;

-- ============================================
-- 附加表：群消息已读表（支持群聊逐人已读）
-- ============================================
DROP TABLE IF EXISTS `group_message_read`;
CREATE TABLE `group_message_read` (
        `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
        `message_id` BIGINT UNSIGNED NOT NULL COMMENT '引用 chat_record.id',
        `user_id` BIGINT UNSIGNED NOT NULL COMMENT '已读用户ID（引用 user.id）',
        `read_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '已读时间',
        PRIMARY KEY (`id`),
        UNIQUE KEY `uk_message_user` (`message_id`,`user_id`),
        KEY `idx_message_id` (`message_id`),
        KEY `idx_user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='群消息已读表';

-- ============================================
-- 附件表：消息附件元数据（图片、文件等）
-- ============================================
DROP TABLE IF EXISTS `attachment`;
CREATE TABLE `attachment` (
        `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '主键ID',
        `message_id` BIGINT UNSIGNED DEFAULT NULL COMMENT '引用 chat_record.id（若为消息附件）',
        `path` VARCHAR(512) NOT NULL COMMENT '存储路径或 URL',
        `mime` VARCHAR(64) DEFAULT NULL COMMENT 'MIME 类型',
        `size` BIGINT UNSIGNED DEFAULT NULL COMMENT '文件大小（字节）',
        `width` INT DEFAULT NULL COMMENT '图片宽度',
        `height` INT DEFAULT NULL COMMENT '图片高度',
        `create_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
        PRIMARY KEY (`id`),
        KEY `idx_message_id` (`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='附件表';

-- ============================================
-- 添加外键约束（增强参照完整性）
-- ============================================
/* 注意：添加外键可能在某些部署场景增加写入开销，若需兼容无外键场景可移除下面部分 */
/*
ALTER TABLE `group_chat` 
    ADD CONSTRAINT `fk_group_chat_creator` FOREIGN KEY (`creator_id`) REFERENCES `user`(`id`) ON DELETE RESTRICT ON UPDATE CASCADE;

ALTER TABLE `group_member`
    ADD CONSTRAINT `fk_group_member_group` FOREIGN KEY (`group_id`) REFERENCES `group_chat`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_group_member_user` FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `friend_relation`
    ADD CONSTRAINT `fk_friend_relation_user` FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_friend_relation_friend` FOREIGN KEY (`friend_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `friend_request`
    ADD CONSTRAINT `fk_friend_request_from_user` FOREIGN KEY (`from_user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_friend_request_to_user` FOREIGN KEY (`to_user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `group_request`
    ADD CONSTRAINT `fk_group_request_group` FOREIGN KEY (`group_id`) REFERENCES `group_chat`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_group_request_from_user` FOREIGN KEY (`from_user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `chat_record`
    ADD CONSTRAINT `fk_chat_record_sender` FOREIGN KEY (`sender_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_chat_record_receiver` FOREIGN KEY (`receiver_id`) REFERENCES `user`(`id`) ON DELETE SET NULL ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_chat_record_group` FOREIGN KEY (`group_id`) REFERENCES `group_chat`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `verification_code`
    ADD CONSTRAINT `uk_verification_code_token` UNIQUE (`captcha_token`);

ALTER TABLE `group_message_read`
    ADD CONSTRAINT `fk_gmr_message` FOREIGN KEY (`message_id`) REFERENCES `chat_record`(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    ADD CONSTRAINT `fk_gmr_user` FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

ALTER TABLE `attachment`
    ADD CONSTRAINT `fk_attachment_message` FOREIGN KEY (`message_id`) REFERENCES `chat_record`(`id`) ON DELETE SET NULL ON UPDATE CASCADE;

SELECT '数据库初始化（含扩展）完成！' AS message;
*/