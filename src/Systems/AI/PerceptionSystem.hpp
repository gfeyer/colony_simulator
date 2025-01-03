#ifndef AI_PERCEPTION_SYSTEM_HPP
#define AI_PERCEPTION_SYSTEM_HPP

#include "Game/GameEntityManager.hpp"

#include "Components/GarissonComponent.hpp"
#include "Components/FactionComponent.hpp"
#include "Components/AIComponent.hpp"

#include "Utils/Logger.hpp"

namespace Systems::AI {

    float getDistanceBetweenEntities(Game::GameEntityManager& entityManager, EntityID entity1, EntityID entity2){
        auto* entity1Transform = entityManager.getEntity(entity1).getComponent<Components::TransformComponent>();
        auto* entity2Transform = entityManager.getEntity(entity2).getComponent<Components::TransformComponent>();

        return sqrtf(powf(entity1Transform->getPosition().x - entity2Transform->getPosition().x, 2) + powf(entity1Transform->getPosition().y - entity2Transform->getPosition().y, 2));
    }
    
    void PerceptionSystem(Game::GameEntityManager& entityManager, float dt){
        Entity& aiEntity = entityManager.getAIEntity();
        auto* aiComp = aiEntity.getComponent<Components::AIComponent>();

        if(!aiComp){
            log_err << "Failed to get aiComponent";
        }

        auto& entities = entityManager.getAllEntities();

        for(auto& [id, entity] : entities){
            auto* garisson = entity.getComponent<Components::GarissonComponent>();
            auto* faction = entity.getComponent<Components::FactionComponent>();

            // Get drone counts in garrisons for faction
            if(garisson && garisson->getDroneCount() > 0 && faction && faction->faction != Components::Faction::NEUTRAL){
                
                aiComp->perception.garissonByDroneCount[id] = garisson->getDroneCount();

                if( faction->faction == Components::Faction::PLAYER_1){
                    aiComp->perception.playerTotalDrones += garisson->getDroneCount();
                    aiComp->perception.playerGarissons.insert(id);
                }
                if(faction->faction == Components::Faction::PLAYER_2){
                    aiComp->perception.aiTotalDrones += garisson->getDroneCount();
                    aiComp->perception.aiGarissons.insert(id);
                }
            }

            // Add in flight drones
            auto* droneComp = entity.getComponent<Components::DroneComponent>();
            if(droneComp && faction && faction->faction != Components::Faction::NEUTRAL){

                if( faction->faction == Components::Faction::PLAYER_1){
                    aiComp->perception.playerTotalDrones += 1;
                }
                if(faction->faction == Components::Faction::PLAYER_2){
                    aiComp->perception.aiTotalDrones += 1;
                }
            }

            // Get total production rate
            auto* factory = entity.getComponent<Components::FactoryComponent>();
            if(factory && factory->droneProductionRate > 0 && faction && faction->faction != Components::Faction::NEUTRAL){

                if( faction->faction == Components::Faction::PLAYER_1){
                    aiComp->perception.playerDroneProductionRate += factory->droneProductionRate;
                }
                if(faction->faction == Components::Faction::PLAYER_2){
                    aiComp->perception.aiDroneProductionRate += factory->droneProductionRate;
                }
            }

            auto* powerPlant = entity.getComponent<Components::PowerPlantComponent>();
            if(powerPlant && powerPlant->capacity > 0 && faction && faction->faction != Components::Faction::NEUTRAL){

                if( faction->faction == Components::Faction::PLAYER_1){
                    aiComp->perception.playerTotalEnergy += powerPlant->capacity;
                }
                if(faction->faction == Components::Faction::PLAYER_2){
                    aiComp->perception.aiTotalEnergy += powerPlant->capacity;
                }
            }
        }

        // Compute the garissonByDistance for ai garissons
        for(auto aiGarissonID : aiComp->perception.aiGarissons){
            auto& aiGarisson = entityManager.getEntity(aiGarissonID);

            for(auto& [targetEntityID, targetEntity] : entities){
                auto* targetGarissonComp = targetEntity.getComponent<Components::GarissonComponent>();
                auto* targetFaction = targetEntity.getComponent<Components::FactionComponent>();

                if(!targetGarissonComp || aiGarissonID == targetEntityID){
                    // consider garissons only
                    continue;
                }

                if(targetFaction && targetFaction->faction == Components::Faction::PLAYER_2){
                    // consider only player 1 and neutral only
                    continue;
                }

                auto distance = getDistanceBetweenEntities(entityManager, aiGarissonID, targetEntityID);
                aiComp->perception.garissonsByDistance[aiGarissonID][distance] = targetEntityID;
                // log_info << "Garisson:" << aiGarissonID << " -> " << targetEntityID << " : " << distance;
            }
        }

        // Get all attack orders
        for(auto& [id, entity] : entities){
            auto *attackOrder = entity.getComponent<Components::AttackOrderComponent>();
            auto* faction = entity.getComponent<Components::FactionComponent>();
            if(attackOrder && faction){
                if(faction->faction == Components::Faction::PLAYER_1){
                    aiComp->perception.playerAttackOrders.insert({attackOrder->origin, attackOrder->target,0.f ,0.f});
                }
                if(faction->faction == Components::Faction::PLAYER_2){
                    aiComp->perception.aiAttackOrders.insert({attackOrder->origin, attackOrder->target, 0.f, 0.f});
                }
            }
        }
    }
}


#endif