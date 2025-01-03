#ifndef COMBAT_SYSTEM_HPP
#define COMBAT_SYSTEM_HPP

#include <unordered_map>
#include <unordered_set>

#include "Core/Entity.hpp"

#include "Components/MoveComponent.hpp"
#include "Components/AttackOrderComponent.hpp"
#include "Components/GarissonComponent.hpp"
#include "Core/Entity.hpp"

#include "Utils/Logger.hpp"

namespace Systems {
        void CombatSystem(Game::GameEntityManager& entityManager, float dt) {

            std::unordered_set<EntityID> toRemove;

            auto& entities = entityManager.getAllEntities();
        
            for (auto& [id, entity] : entities) {
                auto* attackOrder = entity.getComponent<Components::AttackOrderComponent>();
                auto* originGarisson = entity.getComponent<Components::GarissonComponent>();
                auto* drone = entity.getComponent<Components::DroneComponent>();
                auto* move = entity.getComponent<Components::MoveComponent>();

                if (attackOrder && originGarisson) {
                    // log_info << "Garisson has attack order";
                    // Attack order was just palced at a garison
                    // Create drones and send them to the target
                    if (originGarisson->getDroneCount() < 2) {
                        // log_info << "EntityID: " << id << " has no drones, removing attack order";
                        entityManager.removeComponent<Components::AttackOrderComponent>(id);
                        continue;
                    }

                    Entity& originEntity = entityManager.getEntity(attackOrder->origin);
                    Entity& targetEntity = entityManager.getEntity(attackOrder->target);

                    auto* originFaction = originEntity.getComponent<Components::FactionComponent>();
                    auto* targetFaction = targetEntity.getComponent<Components::FactionComponent>();

                    // TODO: insert error msg if originFaction is missing
                    auto dronesUsedForAttack = originGarisson->getDroneCount()-1;

                    for(auto i=0; i < dronesUsedForAttack; i++){
                        EntityID droneID = Game::createDrone(entityManager, std::to_string(i), originFaction->faction);
                        Entity& droneEntity = entityManager.getEntity(droneID);

                        entityManager.addComponent(droneID,Components::AttackOrderComponent{attackOrder->origin, attackOrder->target});

                        sf::Vector2f originPosition = originEntity.getComponent<Components::TransformComponent>()->transform.getPosition();
                        int spread = 25 + (dronesUsedForAttack * 5);
                        spread = std::min(spread, 75);
                        sf::Vector2f randomOffset = sf::Vector2f(
                            rand() % (2 * spread) - spread,
                            rand() % (2 * spread) - spread
                        );

                        auto* droneTransform = droneEntity.getComponent<Components::TransformComponent>();
                        droneTransform->transform.setPosition(originPosition + randomOffset);

                        auto* droneMove = droneEntity.getComponent<Components::MoveComponent>();
                        droneMove->targetPosition = targetEntity.getComponent<Components::TransformComponent>()->transform.getPosition();
                        droneMove->moveToTarget = true;
                    }
                    originGarisson->setDroneCount(1);
                    entityManager.removeComponent<Components::AttackOrderComponent>(id);
                }
                else if (attackOrder && drone && move) {
                    if (!move->moveToTarget) {
                        // Drone Reached destination

                        Entity& originEntity = entityManager.getEntity(attackOrder->origin);
                        Entity& targetEntity = entityManager.getEntity(attackOrder->target);
                        
                        auto* originFaction = entity.getComponent<Components::FactionComponent>(); // Get the faction of the drone, in case the origin entity changed factions
                        auto* targetFaction = targetEntity.getComponent<Components::FactionComponent>();
                        auto* targetGarisson = targetEntity.getComponent<Components::GarissonComponent>();
                        auto* targetShield = targetEntity.getComponent<Components::ShieldComponent>();
        
                        if(!targetShield){
                            log_err << "EntityID: " << id << " has no shield, but has an attack order";
                        }

                        if (targetGarisson && originFaction && targetFaction) {
                            
                            // No matter what, drone entity needs to be removed
                            toRemove.insert(id);

                            auto* gameState = entityManager.getGameStateEntity().getComponent<Components::GameStateComponent>();

                            if(originFaction->faction == targetFaction->faction){
                                // Same faction, park drones
                                targetGarisson->incrementDroneCount();
                                continue; 
                            }
                            
                            // If shield is positive, hit shield and update its value
                            if(targetShield->getShield() > 1.f){
                                targetShield->decrementShield();
                            }else{
                                targetShield->setShield(0.f); 
                            }
                            
                            if(targetShield->getShield() > 0.f){
                                // Shield was hit but still up, attacking player loses drones
                                gameState->playerDrones[originFaction->faction]--;

                            }else if(targetGarisson->getDroneCount() > 0){
                                // Shield is down
                                // Different faction has drones parked
                                // Kill drones
                                targetGarisson->decrementDroneCount();

                                // both players lose drones
                                gameState->playerDrones[originFaction->faction]--;
                                gameState->playerDrones[targetFaction->faction]--;
                            }else{
                                // Different Faction, no shield, no drones, switch factions
                                targetFaction->faction = originFaction->faction;
                                targetGarisson->incrementDroneCount();
                            }
                            
                        }
                    }
                }
            }

            for (const auto& id : toRemove) {
                entityManager.removeEntity(id);
            }
        }

}

#endif // COMBAT_SYSTEM_HPP