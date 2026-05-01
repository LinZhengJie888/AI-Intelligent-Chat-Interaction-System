#pragma once

#include "model/GroupChat.h"
#include "model/GroupMember.h"
#include "module/Database.h"
#include <vector>

class GroupChatDAO {
public:
    GroupChatDAO(Database& db);
    
    bool insert(GroupChat& group);
    bool update(const GroupChat& group);
    bool remove(uint64_t id);
    
    GroupChat* findById(uint64_t id);
    GroupChat* findByGroupId(const std::string& group_id);
    std::vector<GroupChat> findByCreator(uint64_t creator_id);
    
    bool addMember(const GroupMember& member);
    bool removeMember(uint64_t group_id, uint64_t user_id);
    bool updateMember(const GroupMember& member);
    std::vector<GroupMember> getMembers(uint64_t group_id);
    GroupMember* getMember(uint64_t group_id, uint64_t user_id);
    
private:
    Database& db_;
};
