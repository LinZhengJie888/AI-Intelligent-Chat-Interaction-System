-- ============================================
-- 数据库迁移脚本：修复好友相关表的字段类型
-- ============================================

USE ai_chat_system;

-- 1. 修复 friend_relation 表
-- 先删除外键约束（忽略错误如果不存在）
ALTER TABLE `friend_relation` DROP FOREIGN KEY `fk_friend_relation_user`;
ALTER TABLE `friend_relation` DROP FOREIGN KEY `fk_friend_relation_friend`;

-- 修改字段类型为 VARCHAR(32)
ALTER TABLE `friend_relation` MODIFY COLUMN `user_id` VARCHAR(32) NOT NULL COMMENT '用户ID';
ALTER TABLE `friend_relation` MODIFY COLUMN `friend_id` VARCHAR(32) NOT NULL COMMENT '好友用户ID';

-- 重新创建索引
ALTER TABLE `friend_relation` DROP INDEX `uk_user_friend`;
ALTER TABLE `friend_relation` ADD UNIQUE KEY `uk_user_friend` (`user_id`, `friend_id`);
ALTER TABLE `friend_relation` DROP INDEX `idx_friend_id`;
ALTER TABLE `friend_relation` ADD KEY `idx_friend_id` (`friend_id`);

-- 2. 修复 friend_request 表
-- 先删除外键约束
ALTER TABLE `friend_request` DROP FOREIGN KEY `fk_friend_request_from_user`;
ALTER TABLE `friend_request` DROP FOREIGN KEY `fk_friend_request_to_user`;

-- 修改字段类型为 VARCHAR(32)
ALTER TABLE `friend_request` MODIFY COLUMN `from_user_id` VARCHAR(32) NOT NULL COMMENT '请求方用户ID';
ALTER TABLE `friend_request` MODIFY COLUMN `to_user_id` VARCHAR(32) NOT NULL COMMENT '接收方用户ID';

-- 重新创建索引
ALTER TABLE `friend_request` DROP INDEX `idx_from_user`;
ALTER TABLE `friend_request` ADD KEY `idx_from_user` (`from_user_id`);
ALTER TABLE `friend_request` DROP INDEX `idx_to_user`;
ALTER TABLE `friend_request` ADD KEY `idx_to_user` (`to_user_id`);

-- 3. 清空旧数据
TRUNCATE TABLE `friend_relation`;
TRUNCATE TABLE `friend_request`;

SELECT '数据库迁移完成！' AS message;
