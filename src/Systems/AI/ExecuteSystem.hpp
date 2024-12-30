#ifndef AI_EXECUTE_SYSTEM_HPP
#define AI_EXECUTE_SYSTEM_HPP

#include "Game/GameEntityManager.hpp"

namespace Systems::AI {

    void ExecuteSystem(Game::GameEntityManager& entityManager, float dt){
        Entity& aiEntity = entityManager.getAIEntity();
        auto* aiComp = aiEntity.getComponent<Components::AIComponent>();
    
        int attackOrdersExecuted = 0;

        for(auto& [source, target] : aiComp->execute.finalTargets){
            // log_info << "Attack: "<< source << " -> " << target;
            entityManager.addComponent(source, Components::AttackOrderComponent{source, target});
            attackOrdersExecuted++;

            if (attackOrdersExecuted >=2) break;
        }
    }
}

#endif // AI_EXECUTE_SYSTEM_HPP