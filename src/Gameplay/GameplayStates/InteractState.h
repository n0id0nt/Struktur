#pragma once

#include <cstdlib>
#include <algorithm>
//#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/GameContext.h"

#include "Engine/Game/State.h"
#include "Engine/Game/StateManager.h"

#include "engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/CameraSystem.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/Camera.h"

#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"

#include "Gameplay/GameObjects/Player.h"

constexpr static float TEXT_SCROLL_SPEED = 0.02f;

namespace Struktur
{    
	namespace GamePlay
	{
        struct DialogueStep
        {
            std::string dialogue;
            std::vector<std::string> conditionContainsItemVector; // Item must be in inventory to actiavate this step
            std::vector<std::string> conditionExcludeItemVector; // Item must not be in inventory to actiavate this step
            std::vector<std::string> itemAddVector; // Add this to the players inventory
            std::vector<std::string> itemRemoveVector; // Add this to the players inventory
            bool applyCameraShake;
            bool deleteInteractingEntity;
            bool animateText;
            bool exitInteraction;
        };

        struct Interaction
        {
            std::vector<DialogueStep> dialougeVector;
        };

        static std::unordered_map<std::string, Interaction> s_interactionMap = {
            // === NPCs ===
            
            {"Scholar", {{ // Source: Library
                // After Quest Complete (Has Red Master Key)
                {"The knowledge is shared, as it should be.\nTheodore's greatest discovery is now yours to use wisely.", {"Red Master Key"}, {}, {}, {}, false, false, true, true},
                
                // Has All Three Herbs (Moonbell Herb + Fireroot + Stargrass)
                {"Perfect! The herbs of wisdom! Moonbell for illumination, Fireroot for transformation, Stargrass for transcendence. Now I can decode Theodore's cipher!", {"Scholar's Memory Note", "Moonbell Herb", "Fireroot", "Stargrass"}, {}, {"Safe Combination Note", "Red Master Key"}, {"Moonbell Herb", "Fireroot", "Stargrass"}, false, false, true, true},
                
                // Subsequent Meetings (Has Scholar's Memory Note, Missing Herbs)
                {"Have you found the three herbs?\nMoonbell from gardens of memory, Fireroot from laboratories of change, Stargrass from observatories of dream.", {"Scholar's Memory Note"}, {"Safe Combination Note"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Welcome, seeker of knowledge. I solved the greatest theorem of my age, but I never got to share it.\nI have Theodore's encrypted research, but it requires three sacred herbs to decode.", {}, {"Scholar's Memory Note"}, {"Encrypted Note", "Scholar's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Cook", {{ // Source: Kitchen
                // After Quest Complete (Has Cook's Blessing Token)
                {"Family traditions live on through those who remember them. You carry our love with you now.", {"Cook's Blessing Token"}, {}, {}, {}, false, false, true, true},
                
                // Has Complete Recipe (Recipe Fragment A + Recipe Fragment B)
                {"My stars! Grandmother's recipe, whole again! Just like her cookies brought the family together, this will help you find your way home.", {"Cook's Memory Note", "Recipe Fragment A", "Recipe Fragment B"}, {}, {"Green Master Key", "Cook's Blessing Token"}, {"Recipe Fragment A", "Recipe Fragment B"}, false, false, true, true},
                
                // Subsequent Meetings (Has Cook's Memory Note, Incomplete Recipe)
                {"The recipe was torn in half and hidden when the family scattered. Find both pieces, and I'll prepare something that opens more than just appetites.", {"Cook's Memory Note"}, {"Cook's Blessing Token"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Hello dear! Christmas morning, 1967 - the last time we were all together.\nI'll cook you something special if you bring me grandmother's complete recipe.", {}, {"Cook's Memory Note"}, {"Cook's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Knight", {{ // Source: Armory
                // After Trade (Has Guardian's Trial Note)
                {"Honor is kept through deeds, not words. The Guardian awaits your proof of understanding.", {"Guardian's Trial Note"}, {}, {}, {}, false, false, true, true},
                
                // Holding Any Weapon (Ceremonial Sword)
                {"A fine blade! In return, know this: The Guardian in the Vault tests with three symbols - Sun, Moon, and Spiral.\nFind them, face them in order, and prove your worth.", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {"Guardian's Trial Note"}, {"Ceremonial Sword"}, false, false, true, true},
                
                // Subsequent Meetings (Has Knight's Honor Note)
                {"Bring me a weapon, and I'll share what I've observed about this battlefield of memories.", {"Knight's Honor Note"}, {"Guardian's Trial Note"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Stand ready! Sir Edmund Cross, forever prepared for the charge that saved my unit.\nI share knowledge with those who understand honor.", {}, {"Knight's Honor Note"}, {"Knight's Honor Note"}, {}, false, false, true, true},
            }}},

            {"Gardener", {{ // Source: Garden
                // Subsequent Meetings (Has Gardener's Wisdom Note)
                {"Theodore wanted to preserve our perfect moments, but he never understood - love isn't about holding onto the past, it's about planting seeds for the future.", {"Gardener's Wisdom Note"}, {}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Welcome to my sanctuary, where love took root and promises were made.\nI tend to memories as if they were flowers.", {}, {"Gardener's Wisdom Note"}, {"Gardener's Wisdom Note"}, {}, false, false, true, true},
            }}},

            {"Inventor", {{ // Source: Workshop
                // After Trade (Has Inventor's Trade Note)
                {"Sometimes the most important inventions are the ones that bring families together.", {"Inventor's Trade Note"}, {}, {}, {}, false, false, true, true},
                
                // Holding Broken Clockwork
                {"Perfect! These gears have the right temporal resonance.\nHere's something I found - part of an old family recipe.", {"Inventor's Project Note", "Broken Clockwork"}, {}, {"Recipe Fragment B", "Inventor's Trade Note"}, {"Broken Clockwork"}, false, false, true, true},
                
                // Subsequent Meetings (Has Inventor's Project Note, No Clockwork)
                {"Bring me broken clockwork - gears, springs, escapements - and I'll trade you something valuable.", {"Inventor's Project Note"}, {"Inventor's Trade Note"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Ah! A visitor to my workshop! I've been building a Temporal Stabilizer, but I need proper clockwork components.", {}, {"Inventor's Project Note"}, {"Inventor's Project Note"}, {}, false, false, true, true},
            }}},

            {"Guardian", {{ // Source: Vault
                // After Approval (Has Guardian's Approval Note)
                {"The vault's treasures await those who have proven their understanding of memory's true nature.", {"Guardian's Approval Note"}, {}, {}, {}, false, false, true, true},
                
                // Has All Symbol Knowledge (Seen all three symbols)
                {"You've found them all! Sun, Moon, Spiral - the cycle of memory!\nNow face the final test: In my safe lies a key, but only those who understand the Scholar's wisdom may claim it.", {"Guardian's Challenge Note", "Sun Symbol Seen", "Moon Symbol Seen", "Spiral Symbol Seen"}, {}, {"Guardian's Approval Note", "Yellow Master Key"}, {}, false, false, true, true},
                
                // Subsequent Meetings (Has Guardian's Challenge Note, No Symbols)
                {"Prove you understand memory's nature by finding three symbols: the Sun of creation, the Moon of preservation, and the Spiral of transcendence.", {"Guardian's Challenge Note"}, {"Guardian's Approval Note"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Welcome, descendant! I am Theodore Aldrich, creator of this memory palace!\nI've preserved perfect moments within these walls.", {}, {"Guardian's Challenge Note"}, {"Guardian's Challenge Note"}, {}, false, false, true, true},
            }}},

            {"Mage", {{ // Source: Portal Chamber
                // After Redemption (Has Mage's Redemption Note)
                {"You've achieved what I could not - understanding that the future matters more than the past.", {"Mage's Redemption Note"}, {}, {}, {}, false, false, true, true},
                
                // Holding Charged Memory Crystal
                {"Perfect! This crystal contains what could be, not just what was.\nYou understand what Theodore never did. Take this - my redemption.", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {"Purple Master Key", "Mage's Redemption Note"}, {"Charged Memory Crystal"}, false, false, true, true},
                
                // Holding Memory Crystal (Uncharged)
                {"The crystal contains potential, but it needs the Dreamer's touch while starlight watches.\nBring it to one who sleeps while the Observatory is active.", {"Mage's Lament Note", "Memory Crystal"}, {}, {"Crystal Charging Note"}, {}, false, false, true, true},
                
                // Subsequent Meetings (Has Mage's Lament Note, No Crystal)
                {"To prove mastery over this house's memory, you must create the Pattern of Echoes.\nOnly when memories overlap perfectly will the Memory Crystal manifest.", {"Mage's Lament Note"}, {"Crystal Charging Note"}, {}, {}, false, false, true, true},
                
                // First Meeting (Default)
                {"Child, you stand where Theodore's ritual failed. I am Vera Cross, trapped in my moment of greatest failure - arriving too late to stop this catastrophe.", {}, {"Mage's Lament Note"}, {"Mage's Lament Note"}, {}, false, false, true, true},
            }}},

            {"Dreamer", {{ // Source: Bedroom
                // After Charging (Has Dream Blessing Note)
                {"Dreams and starlight together can break even the strongest chains of memory.", {"Dream Blessing Note"}, {}, {}, {}, false, false, true, true},
                
                // Holding Memory Crystal + Observatory Active (need to check West room state in code)
                {"The crystal... yes, I dreamed of this. Let me fill it with dreams of freedom and possibility.", {"Dreamer's Vision Note", "Memory Crystal"}, {}, {"Charged Memory Crystal", "Dream Blessing Note"}, {"Memory Crystal"}, false, false, true, true},
                
                // Interacting While Sleeping / Default
                {"Mmm? I was dreaming of the house's true nature... but it fades like morning mist.\nI can only help when the stars are watching.", {}, {"Dreamer's Vision Note"}, {"Dreamer's Vision Note"}, {}, false, false, true, true},
            }}},

            // === ROOM TRANSFORMATION ITEMS ===

            {"Family Portrait", {{ // Controls North Room, Source: Vault
                // Putback in wrong room
                {"This doesn't belong here. The portrait should return to where you found it.", {}, {"Family Portrait"}, {}, {}, false, false, true, true},
                // Putback in correct room (Vault)
                {"You place the portrait back on the safe.\nThe North Room will no longer be Garden next loop.", {"Family Portrait"}, {}, {}, {"Family Portrait"}, false, false, true, true},
                // Pickup
                {"A photo of happier times before the curse. Carrying it draws you toward natural beauty and growth.\nThe North Room will be Garden next loop.", {}, {"Family Portrait"}, {"Family Portrait"}, {}, false, false, true, true},
            }}},

            {"Broken Clockwork", {{ // Controls North Room, Source: Treasury
                // Putback in wrong room
                {"The clockwork belongs with the other valuables in the Treasury.", {}, {"Broken Clockwork"}, {}, {}, false, false, true, true},
                // Putback in correct room (Treasury)
                {"You return the clockwork to its place.\nThe North Room will no longer be Armory next loop.", {"Broken Clockwork"}, {}, {}, {"Broken Clockwork"}, false, false, true, true},
                // Pickup
                {"Gears and springs from an old timepiece. Carrying it awakens thoughts of honor and duty.\nThe North Room will be Armory next loop.", {}, {"Broken Clockwork"}, {"Broken Clockwork"}, {}, false, false, true, true},
            }}},

            {"Love Letter", {{ // Controls East Room, Source: Library
                // Putback in wrong room
                {"The love letter belongs in the Library where it was kept.", {}, {"Love Letter"}, {}, {}, false, false, true, true},
                // Putback in correct room (Library)
                {"You place the letter back carefully.\nThe East Room will no longer be Kitchen next loop.", {"Love Letter"}, {}, {}, {"Love Letter"}, false, false, true, true},
                // Pickup
                {"Theodore's proposal letter filled with domestic dreams. Carrying it awakens hunger for family comfort.\nThe East Room will be Kitchen next loop.", {}, {"Love Letter"}, {"Love Letter"}, {}, false, false, true, true},
            }}},

            {"Star Chart", {{ // Controls East Room, Source: Portal Chamber
                // Putback in wrong room
                {"The star chart belongs with the mystical apparatus in the Portal Chamber.", {}, {"Star Chart"}, {}, {}, false, false, true, true},
                // Putback in correct room (Portal Chamber)
                {"You return the chart to its place.\nThe East Room will no longer be Lab next loop.", {"Star Chart"}, {}, {}, {"Star Chart"}, false, false, true, true},
                // Pickup
                {"Astronomical maps covered in alchemical symbols. Carrying it stirs scientific curiosity.\nThe East Room will be Lab next loop.", {}, {"Star Chart"}, {"Star Chart"}, {}, false, false, true, true},
            }}},

            {"Recipe Box", {{ // Controls South Room, Source: Library
                // Putback in wrong room
                {"The recipe box belongs in the Library where family documents are kept.", {}, {"Recipe Box"}, {}, {}, false, false, true, true},
                // Putback in correct room (Library)
                {"You return the recipe box to its shelf.\nThe South Room will no longer be Bedroom next loop.", {"Recipe Box"}, {}, {}, {"Recipe Box"}, false, false, true, true},
                // Pickup
                {"A wooden box that once held family recipes. Carrying it awakens longing for personal comfort.\nThe South Room will be Bedroom next loop.", {}, {"Recipe Box"}, {"Recipe Box"}, {}, false, false, true, true},
            }}},

            {"Ritual Candle", {{ // Controls South Room, Source: Portal Chamber
                // Putback in wrong room
                {"The ritual candle must remain with the mystical apparatus.", {}, {"Ritual Candle"}, {}, {}, false, false, true, true},
                // Putback in correct room (Portal Chamber)
                {"You return the candle to the ritual altar.\nThe South Room will no longer be Study next loop.", {"Ritual Candle"}, {}, {}, {"Ritual Candle"}, false, false, true, true},
                // Pickup
                {"Burns with silver flame from Theodore's ritual. Carrying it awakens scholarly investigation.\nThe South Room will be Study next loop.", {}, {"Ritual Candle"}, {"Ritual Candle"}, {}, false, false, true, true},
            }}},

            {"Old Journal", {{ // Controls West Room, Source: Vault
                // Putback in wrong room
                {"The personal journal belongs in the Vault where private things are kept.", {}, {"Old Journal"}, {}, {}, false, false, true, true},
                // Putback in correct room (Vault)
                {"You return the journal to its place.\nThe West Room will no longer be Workshop next loop.", {"Old Journal"}, {}, {}, {"Old Journal"}, false, false, true, true},
                // Pickup
                {"Personal diary filled with daily life and practical concerns. Carrying it awakens the urge to create and build.\nThe West Room will be Workshop next loop.", {}, {"Old Journal"}, {"Old Journal"}, {}, false, false, true, true},
            }}},

            {"Research Notes", {{ // Controls West Room, Source: Treasury
                // Putback in wrong room
                {"The research notes belong with the other valuable documents in the Treasury.", {}, {"Research Notes"}, {}, {}, false, false, true, true},
                // Putback in correct room (Treasury)
                {"You organize the notes back in their place.\nThe West Room will no longer be Observatory next loop.", {"Research Notes"}, {}, {}, {"Research Notes"}, false, false, true, true},
                // Pickup
                {"Cordelia's research into the family curse and astronomical phenomena. Contains the Moon Symbol.\nCarrying it stirs transcendent ambitions. The West Room will be Observatory next loop.", {}, {"Research Notes"}, {"Research Notes", "Moon Symbol Seen"}, {}, false, false, true, true},
            }}},

            // === QUEST ITEMS ===

            {"Moonbell Herb", {{ // Quest item for Scholar
                // Cannot be put back
                {"A silvery herb that chimes with memory. The Scholar seeks this for his cipher.", {}, {"Moonbell Herb"}, {"Moonbell Herb"}, {}, false, false, true, true},
            }}},

            {"Fireroot", {{ // Quest item for Scholar
                // Cannot be put back
                {"A red herb that burns cold. Essential for the Scholar's cipher.", {}, {"Fireroot"}, {"Fireroot"}, {}, false, false, true, true},
            }}},

            {"Stargrass", {{ // Quest item for Scholar
                // Cannot be put back
                {"An ethereal herb that glows with starlight. The final component for the Scholar's ritual.", {}, {"Stargrass"}, {"Stargrass"}, {}, false, false, true, true},
            }}},

            {"Recipe Fragment A", {{ // Quest item for Cook
                // Cannot be put back
                {"Half of grandmother's cookie recipe. The torn edge suggests its partner waits elsewhere.", {}, {"Recipe Fragment A"}, {"Recipe Fragment A"}, {}, false, false, true, true},
            }}},

            {"Recipe Fragment B", {{ // Given by Inventor
                // Cannot be put back - this should never be found as an item, only given by Inventor
            }}},

            {"Memory Crystal", {{ // Quest item for Mage
                // Cannot be put back
                {"A clear crystal that seems to contain swirling potential. It pulses gently, waiting to be filled with dreams and starlight.", {}, {"Memory Crystal"}, {"Memory Crystal"}, {}, false, false, true, true},
            }}},

            // === NON-PICKUPABLE INTERACTABLES ===

            {"Theodore's Safe", {{ // Vault only
                // With combination
                {"You enter 7-3-9. The safe opens with a satisfying click, revealing treasure!", {"Safe Combination Note"}, {"Safe Opened"}, {"Safe Opened"}, {}, true, false, true, true},
                // Without combination
                {"A locked safe requiring a three-number combination. The Scholar might know how to open it.", {}, {"Safe Combination Note"}, {}, {}, false, false, true, true},
            }}},

            {"Sun Symbol Stone", {{ // Armory only
                // First interaction
                {"Carved into the wall: a radiant sun. 'In the beginning, there was light.'", {}, {"Sun Symbol Seen"}, {"Sun Symbol Seen"}, {}, false, false, true, true},
            }}},

            {"Moon Symbol Text", {{ // In Research Notes - this triggers when picking up Research Notes
                // Handled in Research Notes pickup
            }}},

            {"Spiral Symbol Chart", {{ // Observatory only
                // First interaction
                {"Drawn on star charts: an eternal spiral. 'All things return, but changed.'", {}, {"Spiral Symbol Seen"}, {"Spiral Symbol Seen"}, {}, false, false, true, true},
            }}},

            // === SPECIAL QUEST ITEMS ===

            {"Ceremonial Sword", {{ // Can be given to Knight
                // This is a special case - can be taken and given away
                {"A blade of honor. The Knight would appreciate this weapon.", {}, {"Ceremonial Sword"}, {"Ceremonial Sword"}, {}, false, false, true, true},
            }}},

            // Pattern of Echoes trigger - Memory Crystal appears in Study when pattern is active
            {"Pattern of Echoes Active", {{ // Special trigger in Study
                // This would be checked by room logic when all conditions are met
                {"The overlapping memories crystallize into pure potential!", {}, {}, {"Memory Crystal"}, {}, true, false, true, true},
            }}},

            // === FINAL VICTORY ===

            {"Final Pedestals", {{ // Courtyard
                // All keys held
                {"With all four Master Keys in hand, the fountain begins to glow. Four pedestals rise from the water, each awaiting its destined key.", {"Red Master Key", "Green Master Key", "Yellow Master Key", "Purple Master Key"}, {}, {"Victory Achieved"}, {}, true, false, true, true},
                // Missing keys
                {"The fountain remains dormant. You need all four Master Keys to unlock the way home.", {}, {"Red Master Key", "Green Master Key", "Yellow Master Key", "Purple Master Key"}, {}, {}, false, false, true, true},
            }}},

            {"Victory Portal", {{ // Final exit
                {"The exit door unseals with a sound like exhaling. The house's memory loops are broken, and you are free to leave.\nThe trapped souls fade peacefully, finally able to rest.", {"Victory Achieved"}, {}, {}, {}, true, false, true, true},
            }}},
/*
            // Quest Items
            {"Encrypted Note", {{ // Scholar (Library) - Given during first dialogue
                {"The Scholar hands you an encrypted note.\n'Only the three sacred herbs can decode this cipher.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Encrypted Note", {}, {}, {"Encrypted Note"}, {}, false, true, false, false},
            }}},
            {"Recipe Fragment A", {{ // Source: Found in Bedroom under bed
                {"Half of grandmother's cookie recipe.\nThe torn edge suggests its partner waits elsewhere.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Recipe Fragment A", {}, {}, {"Recipe Fragment A"}, {}, false, true, false, false},
            }}},
            {"Recipe Fragment B", {{ // Source: Inventor (Workshop) - Given in exchange for Broken Clockwork
                {"The Inventor trades you the recipe fragment.\n'Family traditions are worth more than gears,' he says.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Recipe Fragment B", {}, {}, {"Recipe Fragment B"}, {}, false, true, false, false},
            }}},
            {"Moonbell Herb", {{ // Source: Found in Garden on plant
                {"The silvery herb chimes softly as you pluck it.\nOne of three for the Scholar's cipher.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Moonbell Herb", {}, {}, {"Moonbell Herb"}, {}, false, true, false, false},
            }}},
            {"Fireroot", {{ // Source: Found in Lab on shelf
                {"The red herb burns cold to the touch.\nThe Scholar seeks this for his ancient knowledge.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Fireroot", {}, {}, {"Fireroot"}, {}, false, true, false, false},
            }}},
            {"Stargrass", {{ // Source: Found in Observatory on windowsill
                {"The ethereal herb glows with starlight.\nThe final component for the Scholar's ritual.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Stargrass", {}, {}, {"Stargrass"}, {}, false, true, false, false},
            }}},
            {"Memory Crystal", {{ // Source: Appears in Study when Pattern of Echoes is active
                {"A crystal forms from the overlapping memories.\nIt pulses with potential, waiting to be charged.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Memory Crystal", {}, {}, {"Memory Crystal"}, {}, false, true, false, false},
            }}},
            {"Broken Clockwork", {{ // Source: Found in Armory in weapon case
                {"Gears and springs from an old timepiece.\nThe Inventor might find this useful.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Broken Clockwork", {}, {}, {"Broken Clockwork"}, {}, false, true, false, false},
            }}},
            {"Safe Combination Note", {{ // Source: Scholar (Library) - Given after bringing all three herbs
                {"The Scholar decodes the cipher.\n'The combination is 7-3-9. Theodore hid his greatest secret in the Vault.'", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Safe Combination Note", {}, {}, {"Safe Combination Note"}, {}, false, true, false, false},
            }}},

            // Room Transformation Items
            {"Ancient Tome", {{ // Source: Found in Library on desk
                {"Theodore's memory research journal.\nTaking this book stirs knowledge-seeking energies.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Ancient Tome", {}, {}, {"Ancient Tome"}, {}, false, true, false, false},
            }}},
            {"Ancient Tome Return", {{ // Source: Found in Library on desk
                {"You replace the tome on its pedestal.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Ancient Tome", {}, {}, {}, {"Ancient Tome"}, false, true, false, false},
            }}},
            {"Ceremonial Sword", {{ // Source: Found in Armory on weapon rack
                {"A blade of honor.\nGrasping it awakens warrior's courage.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Ceremonial Sword", {}, {}, {"Ceremonial Sword"}, {}, false, true, false, false},
            }}},
            {"Ceremonial Sword Return", {{ // Source: Found in Armory on weapon rack
                {"You return the sword to its place.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Ceremonial Sword", {}, {}, {}, {"Ceremonial Sword"}, false, true, false, false},
            }}},
            {"Love Letter", {{ // Source: Found in Garden on bench
                {"Theodore's proposal letter.\nReading it stirs memories of lost romance.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Love Letter", {}, {}, {"Love Letter"}, {}, false, true, false, false},
            }}},
            {"Love Letter Return", {{ // Source: Found in Garden on bench
                {"You carefully place the letter back.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Love Letter", {}, {}, {}, {"Love Letter"}, false, true, false, false},
            }}},
            {"Fresh Bread", {{ // Source: Found in Kitchen on counter
                {"Still-warm bread from Christmas past.\nTaking it awakens hunger for transformation.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Fresh Bread", {}, {}, {"Fresh Bread"}, {}, false, true, false, false},
            }}},
            {"Fresh Bread Return", {{ // Source: Found in Kitchen on counter
                {"You return the bread to its place.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Fresh Bread", {}, {}, {}, {"Fresh Bread"}, false, true, false, false},
            }}},
            {"Memory Vial", {{ // Source: Found in Lab on shelf
                {"Crystallized memories swirl within.\nGrasping it stirs alchemical ambitions.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Memory Vial", {}, {}, {"Memory Vial"}, {}, false, true, false, false},
            }}},
            {"Memory Vial Return", {{ // Source: Found in Lab on shelf
                {"You place the vial back carefully.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Memory Vial", {}, {}, {}, {"Memory Vial"}, false, true, false, false},
            }}},
            {"Gold Coins", {{ // Source: Found in Treasury in chest
                {"Tarnished wealth that couldn't buy happiness.\nTaking them stirs domestic longings.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Gold Coins", {}, {}, {"Gold Coins"}, {}, false, true, false, false},
            }}},
            {"Gold Coins Return", {{ // Source: Found in Treasury in chest
                {"You return the coins to their chest.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Gold Coins", {}, {}, {}, {"Gold Coins"}, false, true, false, false},
            }}},
            {"Personal Diary", {{ // Source: Found in Bedroom on nightstand
                {"Young Cordelia's journal of supernatural encounters.\nReading it awakens scholarly pursuits.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Personal Diary", {}, {}, {"Personal Diary"}, {}, false, true, false, false},
            }}},
            {"Personal Diary Return", {{ // Source: Found in Bedroom on nightstand
                {"You close the diary and place it back.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Personal Diary", {}, {}, {}, {"Personal Diary"}, false, true, false, false},
            }}},
            {"Research Notes", {{ // Source: Found in Bedroom on nightstand
                {"Cordelia's research into the family curse.\nContains the Moon Symbol.\nTaking them stirs protective instincts.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Research Notes", {}, {}, {"Research Notes"}, {}, false, true, false, false},
            }}},
            {"Research Notes Return", {{ // Source: Found in Bedroom on nightstand
                {"You organize the notes back on the desk.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Research Notes", {}, {}, {}, {"Research Notes"}, false, true, false, false},
            }}},
            {"Family Portrait", {{ // Source: Found in Vault on safe
                {"A photo of happier times before the curse.\nTaking it stirs longing for simpler days.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Family Portrait", {}, {}, {"Family Portrait"}, {}, false, true, false, false},
            }}},
            {"Family Portrait Return", {{ // Source: Found in Vault on safe
                {"You place the portrait back with care.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Family Portrait", {}, {}, {}, {"Family Portrait"}, false, true, false, false},
            }}},
            {"Hammer", {{ // Source: Found in Workshop on workbench
                {"A well-used tool of creation.\nGrasping it awakens the urge to build and reach higher.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Hammer", {}, {}, {"Hammer"}, {}, false, true, false, false},
            }}},
            {"Hammer Return", {{ // Source: Found in Workshop on workbench
                {"You place the portrait back with care.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Hammer", {}, {}, {}, {"Hammer"}, false, true, false, false},
            }}},
            {"Telescope", {{ // Source: Found in Observatory on tripod
                {"Reveals distant possibilities and times.\nTaking it stirs transcendent ambitions.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Telescope", {}, {}, {"Telescope"}, {}, false, true, false, false},
            }}},
            {"Telescope Return", {{ // Source: Found in Observatory on tripod
                {"You carefully return the telescope to its mount.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Telescope", {}, {}, {}, {"Telescope"}, false, true, false, false},
            }}},
            {"Ritual Candle", {{ // Source: Found in Portal Chamber on altar
                {"Burns with silver flame from Theodore's ritual.\nTaking it stirs practical needs.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has shifted, but not now - next loop.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Ritual Candle", {}, {}, {"Ritual Candle"}, {}, false, true, false, false},
            }}},
            {"Ritual Candle return", {{ // Source: Found in Portal Chamber on altar
                {"You replace the candle on the altar.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room has settled, no longer in a state to shift.", {}, {}, {}, {}, true, false, false, false},
                {"Put Back Ritual Candle", {}, {}, {}, {"Ritual Candle"}, false, true, false, false},
            }}},

            // Non-pickupable Interactables
            {"Theodore's Safe", {{ // Source: Vault 
                {"You enter 7-3-9. The safe opens with a satisfying click, revealing the Red Master Key!", {"Safe Combination Note"}, {"Red Master Key"}, {}, {}, false, false, false, false},
                {"You Obtained the Red Master Key.", {"Safe Combination Note"}, {"Red Master Key"}, {"Red Master Key"}, {}, false, false, false, true},
                {"You have already opened Theodore's Safe, and taken its contents", {"Safe Combination Note"}, {"Safe Combination Note", "Red Master Key"}, {}, {}, false, false, false, true},
                {"A locked safe requiring a three-number combination. The Scholar might know how to open it.", {}, {}, {}, {}, false, false, false, false},
            }}},
            {"Sun Symbol", {{ // Source: Armory
                {"Carved into the wall: a radiant sun.\n\n\t\t'In the beginning, there was light.", {}, {}, {"Sun Symbol Sketch"}, {}, false, false, false, false},
            }}},
            {"Moon Symbol", {{ // Source: Study
                {"Written in the research notes: a crescent moon.\n\n\t\t'Preservation follows creation.", {}, {}, {"Moon Symbol Sketch"}, {}, false, false, false, false},
            }}},
            {"Spiral Symbol", {{ // Source: Observatory
                {"Drawn on star charts: an eternal spiral.\n\n\t\t'All things return, but changed.", {}, {}, {"Spiral Symbol Sketch"}, {}, false, false, false, false},
            }}},

            // NPC Dialogue
            {"Scholar", {{ // Source: Library
                // After Quest Complete (Has Red Master Key Token)
                {"The knowledge is shared, as it should be.\nTheodore's greatest discovery is now yours to use wisely.", {"Red Master Key"}, {}, {}, {}, false, false, true, true},
                // Has All Three Herbs (Moonbell Herb + Fireroot + Stargrass)
                {"Perfect! The herbs of wisdom! Moonbell for illumination, Fireroot for transformation, Stargrass for transcendence. Now I can decode Theodore's cipher!", {"Scholar's Memory Note", "Moonbell Herb", "Fireroot", "Stargrass"}, {}, {}, {}, false, false, true, false},
                {"Perfect! The herbs of wisdom!", {"Scholar's Memory Note", "Moonbell Herb", "Fireroot", "Stargrass"}, {}, {}, {}, false, false, true, false},
                {"Now I can decode Theodore's cipher!", {"Scholar's Memory Note", "Moonbell Herb", "Fireroot", "Stargrass"}, {}, {"Safe Combination Note"}, {"Moonbell Herb", "Fireroot", "Stargrass"}, false, false, true, true},
                // Subsequent Meetings (Has Scholar's Memory Note, Missing Herbs)
                {"Have you found the three herbs?", {"Scholar's Memory Note"}, {}, {}, {}, false, false, true, false},
                {"Moonbell from gardens of memory,", {"Scholar's Memory Note"}, {}, {}, {}, false, false, true, false},
                {"Fireroot from laboratories of change,", {"Scholar's Memory Note"}, {}, {}, {}, false, false, true, false},
                {"Stargrass from observatories of dream.", {"Scholar's Memory Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome, seeker of knowledge.", {}, {}, {}, {}, false, false, true, false},
                {"I solved the greatest theorem of my age, but I never got to share it.", {}, {}, {}, {}, false, false, true, false},
                {"I have Theodore's encrypted research, but it requires three sacred herbs to decode.", {}, {}, {"Encrypted Note", "Scholar's Memory Note"}, {}, false, false, true, true},
            }}},
            {"Cook", {{ // Source: Kitchen
                // After Quest Complete (Has Red Master Key Token)
                {"Family traditions live on through those who remember them.\nYou carry our love with you now.", {"Green Master Key"}, {}, {}, {}, false, false, true, true},
                // Has Complete Recipe (Recipe Fragment A + Recipe Fragment B)
                {"My stars!\nGrandmother's recipe, whole again!", {"Cook's Memory Note", "Recipe Fragment A", "Recipe Fragment B"}, {}, {}, {}, false, false, true, false},
                {"Just like her cookies brought the family together, this will help you find your way home.", {"Cook's Memory Note", "Recipe Fragment A", "Recipe Fragment B"}, {}, {"Recipe Fragment A", "Recipe Fragment B"}, {"Green Master Key", "Cook's Blessing Token"}, false, false, true, true},
                // Subsequent Meetings (Has Cook's Memory Note, Incomplete Recipe)
                {"The recipe was torn in half and hidden when the family scattered.", {"Cook's Memory Note"}, {}, {}, {}, false, false, true, false},
                {"Find both pieces, and I'll prepare something that opens more than just appetites.", {"Cook's Memory Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Hello dear!", {}, {}, {}, {}, false, false, true, false},
                {"Christmas morning, 1967 - the last time we were all together.", {}, {}, {}, {}, false, false, true, false},
                {"I'll cook you something special if you bring me grandmother's complete recipe.", {}, {}, {"Cook's Memory Note"}, {}, false, false, true, false},
            }}},
            {"Knight", {{ // Source: Armory
                // After Trade (Has Guardian's Trial Note)
                {"Honor is kept through deeds, not words. The Guardian awaits your proof of understanding.", {"Guardian's Trial Note"}, {}, {}, {}, false, false, true, true},
                // Holding Any Weapon (Ceremonial Sword)
                {"A fine blade!", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {}, {}, false, false, true, false},
                {"In return, know this: The Guardian in the Vault tests with three symbols - Sun, Moon, and Spiral.", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {}, {}, false, false, true, false},
                {"Find them, face them in order, and prove your worth.", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {"Guardian's Trial Note"}, {"Ceremonial Sword"}, false, false, true, true},
                // Subsequent Meetings (Has Knight's Honor Note)
                {"Bring me a weapon, and I'll share what I've observed about this battlefield of memories.", {"Knight's Honor Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Stand ready!", {}, {}, {}, {}, false, false, true, false},
                {"Sir Edmund Cross, forever prepared for the charge that saved my unit.", {}, {}, {}, {}, false, false, true, false},
                {"I share knowledge with those who understand honor.", {}, {}, {"Knight's Honor Note"}, {}, false, false, true, false},
            }}},
            {"Gardener", {{ // Source: Garden
                // Subsequent Meetings (Has Gardener's Wisdom Note)
                {"Theodore wanted to preserve our perfect moments,", {"Gardener's Wisdom Note"}, {}, {}, {}, false, false, true, false},
                {"but he never understood - love isn't about holding onto the past,", {"Gardener's Wisdom Note"}, {}, {}, {}, false, false, true, false},
                {"it's about planting seeds for the future.", {"Gardener's Wisdom Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my sanctuary, where love took root and promises were made.", {}, {}, {}, {}, false, false, true, false},
                {"I tend to memories as if they were flowers.", {}, {}, {"Gardener's Wisdom Note"}, {}, false, false, true, false},
            }}},
            {"Inventor", {{ // Source: Workshop
                // After Trade (Has Inventor's Trade Note)
                {"Sometimes the most important inventions are the ones that bring families together.", {"Inventor's Trade Note"}, {}, {}, {}, false, false, true, true},
                // Holding Broken Clockwork
                {"Perfect!", {"Inventor's Project Note", "Broken Clockwork"}, {}, {}, {}, false, false, true, false},
                {"These gears have the right temporal resonance.", {"Inventor's Project Note", "Broken Clockwork"}, {}, {}, {}, false, false, true, false},
                {"Here's something I found - part of an old family recipe.", {"Inventor's Project Note", "Broken Clockwork"}, {}, {"Recipe Fragment B", "Inventor's Trade Note"}, {"Broken Clockwork"}, false, false, true, true},
                // Subsequent Meetings (Has Inventor's Project Note, No Clockwork)
                {"Bring me broken clockwork - gears, springs, escapements - and I'll trade you something valuable.", {"Inventor's Project Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Ah! A visitor to my workshop!", {}, {}, {}, {}, false, false, true, false},
                {"I've been building a Temporal Stabilizer,", {}, {}, {}, {}, false, false, true, false},
                {"but I need proper clockwork components.", {}, {}, {"Inventor's Project Note"}, {}, false, false, true, false},
            }}},
            {"Guardian", {{ // Source: Vault
                // After Approval (Has Guardian's Approval Note)
                {"The vault's treasures await those who have proven their understanding of memory's true nature.", {"Guardian's Approval Note"}, {}, {}, {}, false, false, true, true},
                // Has All Symbol Knowledge (Seen all three symbols)
                {"You've found them all!", {"Guardian's Challenge Note", "Sun Symbol Sketch", "Moon Symbol Sketch", "Spiral Symbol Sketch"}, {}, {}, {}, false, false, true, false},
                {"Sun, Moon, Spiral - the cycle of memory!", {"Guardian's Challenge Note", "Sun Symbol Sketch", "Moon Symbol Sketch", "Spiral Symbol Sketch"}, {}, {}, {}, false, false, true, false},
                {"Now face the final test:\nIn my safe lies a key, but only those who understand the Scholar's wisdom may claim it.", {"Guardian's Challenge Note", "Sun Symbol Sketch", "Moon Symbol Sketch", "Spiral Symbol Sketch"}, {}, {"Guardian's Approval Note"}, {}, false, false, true, true},
                // Subsequent Meetings (Has Guardian's Challenge Note, No Symbols)
                {"Prove you understand memory's nature by finding three symbols:\n\nthe Sun of creation, the Moon of preservation, and the Spiral of transcendence.", {"Guardian's Challenge Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome, descendant!", {}, {}, {}, {}, false, false, true, false},
                {"I am Theodore Aldrich, creator of this memory palace!", {}, {}, {}, {}, false, false, true, false},
                {"I've preserved perfect moments within these walls.", {}, {}, {"Guardian's Challenge Note"}, {}, false, false, true, false},
            }}},
            {"Mage", {{ // Source: Portal Chamber 
                // After Redemption (Has Mage's Redemption Note)
                {"You've achieved what I could not - understanding that the future matters more than the past.", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {}, {}, false, false, true, true},
                // Holding Charged Memory Crystal
                {"Perfect!", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {}, {}, false, false, true, false},
                {"This crystal contains what could be, not just what was.", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {}, {}, false, false, true, false},
                {"You understand what Theodore never did. Take this - my redemption.", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {"Purple Master Key", "Mage's Redemption Note"}, {"Charged Memory Crystal"}, false, false, true, true},
                // Holding Memory Crystal (Uncharged)
                {"The crystal contains potential,\nbut it needs the Dreamer's touch while starlight watches.", {"Mage's Lament Note", "Memory Crystal"}, {}, {}, {}, false, false, true, false},
                {"Bring it to one who sleeps while the Observatory is active.", {"Mage's Lament Note", "Memory Crystal"}, {}, {"Crystal Charging Note"}, {}, false, false, true, true},
                // Subsequent Meetings (Has Mage's Lament Note, No Crystal)
                {"To prove mastery over this house's memory, you must create the Pattern of Echoes.", {"Mage's Lament Note"}, {}, {}, {}, false, false, true, false},
                {"Only when memories overlap perfectly will the Memory Crystal manifest.", {"Mage's Lament Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Child, you stand where Theodore's ritual failed.", {}, {}, {}, {}, false, false, true, false},
                {"I am Vera Cross,\ntrapped in my moment of greatest failure - arriving too late to stop this catastrophe.", {}, {}, {"Mage's Lament Note"}, {}, false, false, true, false},
            }}},
            {"Dreamer", {{ // Source: Bedroom 
                // After Charging (Has Dream Blessing Note)
                {"Dreams and starlight together can break even the strongest chains of memory.", {"Dream Blessing Note"}, {}, {}, {}, false, false, true, false},
                // Holding Memory Crystal + Observatory Active - TODO test that observatory is an active room - COULD give player the rooms in inventory and not show?
                {"The crystal... yes, I dreamed of this.", {"Memory Crystal"}, {}, {}, {}, false, false, true, false},
                {"Let me fill it with dreams of freedom and possibility.", {"Memory Crystal"}, {}, {"Charged Memory Crystal", "Dream Blessing Note"}, {"Memory Crystal"}, false, false, true, true},
                // Interacting While Sleeping
                {"Mmm? I was dreaming of the house's true nature...\n\n\tbut it fades like morning mist.", {}, {}, {}, {}, false, false, true, false},
                {"I can only help when the stars are watching.", {}, {}, {"Dreamer's Vision Note "}, {}, false, false, true, false},
            }}},*/
        };

		class InteractState : public GameResource::IState
		{
        private:
            UI::UIPanel* m_screenPanel;
            UI::UILabel* m_dialogueLabel;
            UI::UILabel* m_continueDialogueLabel;

            entt::entity m_interactingEntity = entt::null;

            Interaction m_interaction;

            bool m_dialogueSrolling = false;
            int m_interactionStep = 0;
            std::string m_currentString;

            double m_currentDialogueStartTime = 0.0;

        public:
            InteractState(entt::entity interactingEntity) : m_interactingEntity(interactingEntity) {}

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // Create Call the interact function and get the player to interact with

                Core::GameData& gameDate = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");

                // UI layout
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                m_screenPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0, 0}, glm::vec2{gameDate.screenWidth, gameDate.screenHeight}, glm::vec2{0, 0});
                m_screenPanel->SetBackgroundColor(BLANK); // don't render this
                m_screenPanel->SetBorderColor(BLANK);

                auto* textBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, -50}, glm::vec2{0.5f, 1.0f}, glm::vec2{700, 150}, glm::vec2{0, 0})));
                textBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 1.0f});
                textBackgroundPanel->SetBackgroundColor(DARKGRAY);
                textBackgroundPanel->SetBorderColor(WHITE);
                textBackgroundPanel->SetBorderWidth(2.0f);

                m_dialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 20, 40 }, glm::vec2{ 0, 0 }, "", 20.0f)));
                m_dialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ -20, -20 }, glm::vec2{ 1, 1 }, "Continue", 20.0f)));
                m_continueDialogueLabel->SetAnchorPoint(glm::vec2{1.0f, 1.0f});
                m_continueDialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel->SetVisible(false);

                // Set State variables
                Component::Interactable interactable = registry.get<Component::Interactable>(m_interactingEntity);

                m_interaction = s_interactionMap[interactable.interactionId];
                SetInteractionStep(context, 0);
            }

            void Update(GameContext& context, GameResource::StateManager& stateManager) override
            {
                Core::GameData& gameDate = context.GetGameData();
                Core::Input& input = context.GetInput();

                double gameTime = gameDate.gameTime;

                bool inputInteract = input.IsInputJustReleased("Interact");
                    
                if (inputInteract)
                {
                    if (m_dialogueSrolling)
                    {
                        m_dialogueSrolling = false;
                        m_continueDialogueLabel->SetVisible(true);
                    }
                    else if (m_interaction.dialougeVector.size() > m_interactionStep + 1)
                    {
                        SetInteractionStep(context, m_interactionStep + 1);
                        m_continueDialogueLabel->SetVisible(false);
                    }
                    else
                    {
                        stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                        return; // we are finished with the interaction.
                    }
                }

                int numberOfCharactersToDraw = m_dialogueSrolling ? (int)std::floor((gameTime - m_currentDialogueStartTime) / TEXT_SCROLL_SPEED) : m_currentString.length();
                if (numberOfCharactersToDraw >= m_currentString.length())
                {
                    numberOfCharactersToDraw = m_currentString.length();
                    m_dialogueSrolling = false;
                    m_continueDialogueLabel->SetVisible(true);
                }
                std::string subString = m_currentString.substr(0, numberOfCharactersToDraw);
                m_dialogueLabel->SetText(subString);
                // animate the text scrolling, Interact with items, add items to inventory, delete items from scene
            }
            void Render(GameContext& context, GameResource::StateManager& stateManager) override {}
            void Exit(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // delete all players
                // delete all UI
				UI::UIManager& uiManager = context.GetUIManager();
                uiManager.RemoveElement(m_screenPanel);
            }

            std::string GetStateName() const override { return std::string(typeid(InteractState).name()); }

            void SetInteractionStep(GameContext& context, int stepIndex)
            {
                Core::GameData& gameDate = context.GetGameData();
                Inventory& inventory = context.GetInventory();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                DialogueStep step = m_interaction.dialougeVector[stepIndex];
                m_currentString = step.dialogue;
                m_dialogueSrolling = step.animateText;
                m_interactionStep = stepIndex;
                m_currentDialogueStartTime = gameDate.gameTime;

                for (auto& item : step.itemAddVector)
                {
                    if(std::find(inventory.begin(), inventory.end(), item) == inventory.end())
                    {
                        inventory.push_back(item);
                    }
                }

                for (auto& item : step.itemRemoveVector)
                {
                    inventory.erase(std::find(inventory.begin(), inventory.end(), item));
                }

                if (step.deleteInteractingEntity)
                {
                    gameObjectManager.DestroyGameObject(context, m_interactingEntity);
                }

                if (step.applyCameraShake)
                {
                    entt::registry& registry = context.GetRegistry();
                    System::SystemManager& systemManager = context.GetSystemManager();
                    System::CameraSystem& cameraSystem = systemManager.GetSystem<System::CameraSystem>();

                    auto view = registry.view<Component::Camera>();
                    for (auto [entity, cameraComponent] : view.each())
                    {
                        cameraSystem.AddCameraTrauma(context, entity, 0.4f);
                    }
                }
            }
        };
    }
}
