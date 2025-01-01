#ifndef AI_PLAN_SYSTEM_HPP
#define AI_PLAN_SYSTEM_HPP

#include <map>

#include "Game/GameEntityManager.hpp"

#include "Utils/Logger.hpp"
#include "Utils/Random.hpp"
#include "Config.hpp"

namespace Systems::AI {

    enum class Strategy : unsigned int {
        None = 0,
        ENERGY = 1,
        PRODUCTION = 2,
        DEFEND = 3,
        ATTACK = 4,
        DISTANCE = 5,
    };

    float computeAttackCost(Game::GameEntityManager& entityManager, EntityID targetEntityID, float distance) {
        // returns how many drones it would take to conquer the target

        auto* targetGarisson = entityManager.getEntity(targetEntityID).getComponent<Components::GarissonComponent>();
        auto* targetShield = entityManager.getEntity(targetEntityID).getComponent<Components::ShieldComponent>();

        // compute drone cost
        auto droneCost = targetGarisson->getDroneCount();
        
        // compute shield cost
        auto currentShield = targetShield->currentShield;

        // compute shield regen cost (for drone travel time)
        auto timeToReachTarget = distance / Config::DRONE_SPEED;
        auto shieldRegenRate = targetShield->regenRate;
        auto shieldRegenCost = timeToReachTarget * shieldRegenRate;

        return droneCost + currentShield + shieldRegenCost;
    }

    std::unordered_map<Strategy, float> computeStrategyPriorities(Game::GameEntityManager& entityManager) {
        std::unordered_map<Strategy, float> priorities = {
            {Strategy::ENERGY, 0.f},
            {Strategy::PRODUCTION, 0.f},
            {Strategy::DEFEND, 0.f},
            {Strategy::ATTACK, 0.f},
        };
        
        Entity& aiEntity = entityManager.getAIEntity();
        auto* aiComp = aiEntity.getComponent<Components::AIComponent>();

        // Compute total droens in 5 seconds if no attack planned
        float totalDrones = 0.f;
        totalDrones += aiComp->perception.aiTotalDrones;
        // totalDrones += aiComp->perception.aiDroneProductionRate * 5.f;
        float aiTotalEnergy = aiComp->perception.aiTotalEnergy;
        float droneToEnergyRatio = totalDrones / aiTotalEnergy;

        // log_info << "droneEnergyRatio: " << droneToEnergyRatio << ", totalDrones: " << totalDrones << ", aiTotalEnergy: " << aiTotalEnergy;

        if(aiTotalEnergy < 20){
            //  log_info << "energy < 21, ENERGY";
            priorities[Strategy::ENERGY] = 1.f;
        }else if(droneToEnergyRatio < 0.5f){
            //  log_info << "droneToEnergyRatio < 0.5, PRODUCTION, " << droneToEnergyRatio;
            priorities[Strategy::PRODUCTION] = 1.f;
        }else if(droneToEnergyRatio > 0.9f){
            priorities[Strategy::ENERGY] = 1.f;
        }
        else{
            // log_info << "DISTANCE";
            priorities[Strategy::DISTANCE] = 1.f;
        }

        // TODO: add Attack/defend strategy

        return priorities;
    }

    void logStrategy(Strategy strategy) {
        // TODO: overload ostream operator?
        switch (strategy)
        {
        case Strategy::ENERGY:
            log_info << "Strategy: Energy";
            break;
        case Strategy::PRODUCTION:
            log_info << "Strategy: Production";
            break;
        case Strategy::DEFEND:
            log_info << "Strategy: Defend";
            break;
        case Strategy::ATTACK:
            log_info << "Strategy: Attack";
            break;
        case Strategy::DISTANCE:
            log_info << "Strategy: Distance";
            break;
        default:
            break;
        };
    }

    void PlanSystem(Game::GameEntityManager& entityManager, float dt){
        Entity& aiEntity = entityManager.getAIEntity();
        auto* aiComp = aiEntity.getComponent<Components::AIComponent>();

        if(!aiComp){
            log_err << "Failed to get aiComponent";
        }

        // Declare local vars
        std::map<float, Components::EntityIDPair> potentialSuccesfulSingleAttackTargetsByDistance; 
        std::map<float, Components::EntityIDPair> potentialFailedSingleAttackTargetsByDistance;

        // Strategy: need energy or more factories?
        auto priorities = computeStrategyPriorities(entityManager);
        auto strategy = std::max_element(priorities.begin(), priorities.end(), [](const std::pair<Strategy, float>& a, const std::pair<Strategy, float>& b) { return a.second < b.second; })->first;

        // logStrategy(strategy);

        // Plan: Check if any single garisson can conquer an adjacent target and save it
        // if not, save it to potential failed attacks
        for(auto it = aiComp->perception.garissonsByDistance.begin(); it != aiComp->perception.garissonsByDistance.end(); it++){
            auto originGarissonID = it->first;

            for(auto [distance, targetEntityID] : it->second){
                float costForSuccesfulAttack = computeAttackCost(entityManager, targetEntityID, distance);
                costForSuccesfulAttack += 2.f; // add some buffer

                auto droneCountAtThisGarisson = aiComp->perception.garissonByDroneCount.at(originGarissonID);

                auto* originTransform = entityManager.getEntity(originGarissonID).getComponent<Components::TransformComponent>();
                auto* targetTransform = entityManager.getEntity(targetEntityID).getComponent<Components::TransformComponent>();

                if(droneCountAtThisGarisson > costForSuccesfulAttack){
                    // log_info << "Can conquer " << targetEntityID << " from " << originGarissonID << ", dist: " << distance;
                    potentialSuccesfulSingleAttackTargetsByDistance[distance] = {originGarissonID, targetEntityID};
                }else{
                    potentialFailedSingleAttackTargetsByDistance[distance] = {originGarissonID, targetEntityID};
                }
            }
        }

        bool submittedAnAttackOrder = false;
        if(!potentialSuccesfulSingleAttackTargetsByDistance.empty()){
            // logStrategy(strategy);
            // implement the strategy 
            // scan through potential targets and chooe a target
            for(auto& [distance, entityPair] : potentialSuccesfulSingleAttackTargetsByDistance){
                auto [source, target] = entityPair;

                // log_info << "dist: " << distance << " source: " << source << " target: " << target;

                auto* targetPowerPlantComp = entityManager.getComponent<Components::PowerPlantComponent>(target);
                auto* targetFactoryComp = entityManager.getComponent<Components::FactoryComponent>(target);

                // Explicit local variable (macOS compiler workaround)
                EntityID targetID = target; 

                // If orders to this target are already issued, don't issue them again
                auto found_target = std::find_if(
                    aiComp->perception.aiAttackOrders.begin(),
                    aiComp->perception.aiAttackOrders.end(),
                    [targetID](const Components::EntityIDPair& pair) {
                        return pair.target == targetID;
                    }
                );
                if(found_target != aiComp->perception.aiAttackOrders.end()){
                    // log_info << "Already issued attack orders to this target";
                    continue;
                }

                // If orders from this source are already issued, don't issue them again
                EntityID sourceID = source;
                auto found_source = std::find_if(
                    aiComp->perception.aiAttackOrders.begin(),
                    aiComp->perception.aiAttackOrders.end(),
                    [sourceID](const Components::EntityIDPair& pair) {
                        return pair.source == sourceID;
                    }
                );
                if(found_source != aiComp->perception.aiAttackOrders.end()){
                    // log_info << "Already issued attack orders from this source";
                    continue;
                }

                // If the distance between the source and target is too large, don't attack
                if(distance > Config::AI_MAX_DISTANCE_TO_ATTACK){
                    // log_info << "Distance between source and target is too large, skipping";
                    continue;
                }

                if(targetPowerPlantComp && strategy == Strategy::ENERGY){
                    // issue orders to attack
                    aiComp->execute.finalTargets.push_back({source, target});
                    aiComp->perception.aiAttackOrders.insert({source, target});
                    submittedAnAttackOrder = true;

                    // Debug symbols
                    auto* originTransform = entityManager.getEntity(source).getComponent<Components::TransformComponent>();
                    auto* targetTransform = entityManager.getEntity(target).getComponent<Components::TransformComponent>();
                    aiComp->debug.yellowDebugTargets.push_back(originTransform->getPosition());
                    aiComp->debug.pinkDebugTargets.push_back(targetTransform->getPosition());

                }else if(targetFactoryComp && strategy == Strategy::PRODUCTION){
                    // issue orders to attack
                    aiComp->execute.finalTargets.push_back({source, target});
                    aiComp->perception.aiAttackOrders.insert({source, target});
                    submittedAnAttackOrder = true;

                    // Debug symbols
                    auto* originTransform = entityManager.getEntity(source).getComponent<Components::TransformComponent>();
                    auto* targetTransform = entityManager.getEntity(target).getComponent<Components::TransformComponent>();
                    aiComp->debug.yellowDebugTargets.push_back(originTransform->getPosition());
                    aiComp->debug.pinkDebugTargets.push_back(targetTransform->getPosition());
                }else if(strategy == Strategy::DISTANCE && (targetPowerPlantComp || targetFactoryComp)){
                    // issue orders to attack
                    aiComp->execute.finalTargets.push_back({source, target});
                    aiComp->perception.aiAttackOrders.insert({source, target});
                    submittedAnAttackOrder = true;

                    // Debug symbols
                    auto* originTransform = entityManager.getEntity(source).getComponent<Components::TransformComponent>();
                    auto* targetTransform = entityManager.getEntity(target).getComponent<Components::TransformComponent>();
                    aiComp->debug.yellowDebugTargets.push_back(originTransform->getPosition());
                    aiComp->debug.pinkDebugTargets.push_back(targetTransform->getPosition());
                }
            }
        }

        // Plan: If no garisson alone can conquer adjacent targets, compute collective plan
        if (potentialSuccesfulSingleAttackTargetsByDistance.empty() || submittedAnAttackOrder == false) {
            // 1. Consolidate into the closest ai target 

            auto& [distance, pair] = *potentialFailedSingleAttackTargetsByDistance.begin();
            auto [consolidateSource, target] = pair;

            for(auto& garisson : aiComp->perception.aiGarissons){
                if(garisson == consolidateSource) continue;

                aiComp->execute.finalTargets.push_back({garisson, consolidateSource});
            }
        }
    }
}

#endif // AI_PLAN_SYSTEM_HPP