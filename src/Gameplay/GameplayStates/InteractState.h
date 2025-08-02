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
            
            // ===== NPC DIALOGUES =====
            
            {"Scholar", {{ // Source: Library
                // After Quest Complete (Has Red Master Key)
                {"The knowledge is shared, as it should be.\nTheodore's greatest discovery is now yours to use wisely.", {"Red Master Key"}, {}, {}, {}, false, false, true, true},
                // Has All Three Herbs (Moonbell Herb + Fireroot + Stargrass)
                {"Perfect! The herbs of wisdom! Moonbell for illumination, Fireroot for transformation, Stargrass for transcendence.\nNow I can decode Theodore's cipher!", {"Scholar's Memory Note", "Moonbell Herb", "Fireroot", "Stargrass"}, {}, {"Safe Combination Note", "Red Master Key"}, {"Moonbell Herb", "Fireroot", "Stargrass"}, false, false, true, true},
                // Subsequent Meetings (Has Scholar's Memory Note, Missing Herbs)
                {"Have you found the three herbs?\nMoonbell from gardens of memory, Fireroot from laboratories of change, Stargrass from observatories of dream.", {"Scholar's Memory Note"}, {"Moonbell Herb", "Fireroot", "Stargrass"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome, seeker of knowledge.\nI solved the greatest theorem of my age, but I never got to share it.", {}, {"Scholar's Memory Note"}, {}, {}, false, false, true, false},
                {"I have Theodore's encrypted research, but it requires three sacred herbs to decode.", {}, {"Scholar's Memory Note"}, {"Encrypted Note", "Scholar's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Cook", {{ // Source: Kitchen
                // After Quest Complete (Has Cook's Blessing Token)
                {"Family traditions live on through those who remember them.\nYou carry our love with you now.", {"Cook's Blessing Token"}, {}, {}, {}, false, false, true, true},
                // Has Complete Recipe (Recipe Fragment A + Recipe Fragment B)
                {"My stars! Grandmother's recipe, whole again!\nJust like her cookies brought the family together, this will help you find your way home.", {"Cook's Memory Note", "Recipe Fragment A", "Recipe Fragment B"}, {}, {"Green Master Key", "Cook's Blessing Token"}, {"Recipe Fragment A", "Recipe Fragment B"}, false, false, true, true},
                // Subsequent Meetings (Has Cook's Memory Note, Incomplete Recipe)
                {"The recipe was torn in half and hidden when the family scattered.\nFind both pieces, and I'll prepare something that opens more than just appetites.", {"Cook's Memory Note"}, {"Recipe Fragment A", "Recipe Fragment B"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Hello dear! Christmas morning, 1967 - the last time we were all together.", {}, {"Cook's Memory Note"}, {}, {}, false, false, true, false},
                {"I'll cook you something special if you bring me grandmother's complete recipe.", {}, {"Cook's Memory Note"}, {"Cook's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Knight", {{ // Source: Armory
                // After Trade (Has Guardian's Trial Note)
                {"Honor is kept through deeds, not words.\nThe Guardian awaits your proof of understanding.", {"Guardian's Trial Note"}, {}, {}, {}, false, false, true, true},
                // Holding Any Weapon (Ceremonial Sword)
                {"A fine blade! In return, know this:\nThe Guardian in the Vault tests with three symbols - Sun, Moon, and Spiral.", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {"Guardian's Trial Note"}, {"Ceremonial Sword"}, false, false, true, false},
                {"Find them, face them in order, and prove your worth.", {"Knight's Honor Note", "Ceremonial Sword"}, {}, {}, {}, false, false, true, true},
                // Subsequent Meetings (Has Knight's Honor Note)
                {"Bring me a weapon, and I'll share what I've observed about this battlefield of memories.", {"Knight's Honor Note"}, {"Ceremonial Sword"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Stand ready! Sir Edmund Cross, forever prepared for the charge that saved my unit.", {}, {"Knight's Honor Note"}, {}, {}, false, false, true, false},
                {"I share knowledge with those who understand honor.", {}, {"Knight's Honor Note"}, {"Knight's Honor Note"}, {}, false, false, true, true},
            }}},

            {"Inventor", {{ // Source: Workshop
                // After Trade (Has Inventor's Trade Note)
                {"Sometimes the most important inventions are the ones that bring families together.", {"Inventor's Trade Note"}, {}, {}, {}, false, false, true, true},
                // Holding Broken Clockwork
                {"Perfect! These gears have the right temporal resonance.\nHere's something I found - part of an old family recipe.", {"Inventor's Project Note", "Broken Clockwork"}, {}, {"Recipe Fragment B", "Inventor's Trade Note"}, {"Broken Clockwork"}, false, false, true, true},
                // Subsequent Meetings (Has Inventor's Project Note, No Clockwork)
                {"Bring me broken clockwork - gears, springs, escapements - and I'll trade you something valuable.", {"Inventor's Project Note"}, {"Broken Clockwork"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Ah! A visitor to my workshop!\nI've been building a Temporal Stabilizer, but I need proper clockwork components.", {}, {"Inventor's Project Note"}, {"Inventor's Project Note"}, {}, false, false, true, true},
            }}},

            {"Guardian", {{ // Source: Vault
                // After Approval (Has Guardian's Approval Note)
                {"The vault's treasures await those who have proven their understanding of memory's true nature.", {"Guardian's Approval Note"}, {}, {}, {}, false, false, true, true},
                // Has All Symbol Knowledge (Seen all three symbols)
                {"You've found them all! Sun, Moon, Spiral - the cycle of memory!\nNow face the final test: In my safe lies a key, but only those who understand the Scholar's wisdom may claim it.", {"Guardian's Challenge Note", "Sun Symbol Seen", "Moon Symbol Seen", "Spiral Symbol Seen"}, {}, {"Guardian's Approval Note"}, {}, false, false, true, true},
                // Subsequent Meetings (Has Guardian's Challenge Note, No Symbols)
                {"Prove you understand memory's nature by finding three symbols:\nthe Sun of creation, the Moon of preservation, and the Spiral of transcendence.", {"Guardian's Challenge Note"}, {"Sun Symbol Seen", "Moon Symbol Seen", "Spiral Symbol Seen"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome, descendant! I am Theodore Aldrich, creator of this memory palace!", {}, {"Guardian's Challenge Note"}, {}, {}, false, false, true, false},
                {"I've preserved perfect moments within these walls.", {}, {"Guardian's Challenge Note"}, {"Guardian's Challenge Note"}, {}, false, false, true, true},
            }}},

            {"Mage", {{ // Source: Portal Chamber
                // After Redemption (Has Mage's Redemption Note)
                {"You've achieved what I could not - understanding that the future matters more than the past.", {"Mage's Redemption Note"}, {}, {}, {}, false, false, true, true},
                // Holding Charged Memory Crystal
                {"Perfect! This crystal contains what could be, not just what was.\nYou understand what Theodore never did. Take this - my redemption.", {"Mage's Lament Note", "Charged Memory Crystal"}, {}, {"Purple Master Key", "Mage's Redemption Note"}, {"Charged Memory Crystal"}, false, false, true, true},
                // Holding Memory Crystal (Uncharged)
                {"The crystal contains potential, but it needs the Dreamer's touch while starlight watches.\nBring it to one who sleeps while the Observatory is active.", {"Mage's Lament Note", "Memory Crystal"}, {"Charged Memory Crystal"}, {"Crystal Charging Note"}, {}, false, false, true, true},
                // Subsequent Meetings (Has Mage's Lament Note, No Crystal)
                {"To prove mastery over this house's memory, you must create the Pattern of Echoes.\nOnly when memories overlap perfectly will the Memory Crystal manifest.", {"Mage's Lament Note"}, {"Memory Crystal"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Child, you stand where Theodore's ritual failed.\nI am Vera Cross, trapped in my moment of greatest failure - arriving too late to stop this catastrophe.", {}, {"Mage's Lament Note"}, {"Mage's Lament Note"}, {}, false, false, true, true},
            }}},

            {"Dreamer", {{ // Source: Bedroom
                // After Charging (Has Dream Blessing Note)
                {"Dreams and starlight together can break even the strongest chains of memory.", {"Dream Blessing Note"}, {}, {}, {}, false, false, true, true},
                // Holding Memory Crystal + Observatory Active (Research Notes in inventory)
                {"The crystal... yes, I dreamed of this.\nLet me fill it with dreams of freedom and possibility.", {"Dreamer's Vision Note", "Memory Crystal", "Research Notes"}, {}, {"Charged Memory Crystal", "Dream Blessing Note"}, {"Memory Crystal"}, false, false, true, true},
                // Interacting While Sleeping
                {"Mmm? I was dreaming of the house's true nature... but it fades like morning mist.", {}, {"Dreamer's Vision Note"}, {}, {}, false, false, true, false},
                {"I can only help when the stars are watching.", {}, {"Dreamer's Vision Note"}, {"Dreamer's Vision Note"}, {}, false, false, true, true},
            }}},

            {"Gardener", {{ // Source: Garden
                // Subsequent Meetings (Has Gardener's Wisdom Note)
                {"Theodore wanted to preserve our perfect moments, but he never understood -\nlove isn't about holding onto the past, it's about planting seeds for the future.", {"Gardener's Wisdom Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my sanctuary, where love took root and promises were made.", {}, {"Gardener's Wisdom Note"}, {}, {}, false, false, true, false},
                {"I tend to memories as if they were flowers.", {}, {"Gardener's Wisdom Note"}, {"Gardener's Wisdom Note"}, {}, false, false, true, true},
            }}},

            {"Alchemist", {{ // Source: Lab
                // Subsequent Meetings (Has Alchemist's Knowledge Note)
                {"We thought we were creating paradise - a place where perfect moments never ended.\nInstead, we built a prison where perfect moments never begin.", {"Alchemist's Knowledge Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Fascinating! Another visitor to Theodore's grand experiment!", {}, {"Alchemist's Knowledge Note"}, {}, {}, false, false, true, false},
                {"I'm trapped in the moment I discovered the perfect formula for crystallizing human memories.\nThe implications were... overwhelming.", {}, {"Alchemist's Knowledge Note"}, {"Alchemist's Knowledge Note"}, {}, false, false, true, true},
            }}},

            {"Merchant", {{ // Source: Treasury
                // After Transaction (Has Merchant's Deal Note)
                {"Family traditions, passed down through generations - that's true wealth.\nSome things are more valuable than gold.", {"Merchant's Deal Note"}, {}, {}, {}, false, false, true, true},
                // Holding Complete Recipe (Recipe Fragment A + Recipe Fragment B)
                {"Grandmother's recipe! This proves some things are more valuable than gold.", {"Merchant's Wisdom Note", "Recipe Fragment A", "Recipe Fragment B"}, {}, {"Silver Key", "Merchant's Deal Note"}, {}, false, false, true, true},
                // Subsequent Meetings (Has Merchant's Wisdom Note)
                {"I deal in valuable exchanges.\nProve to me that family traditions have worth - show me a complete family recipe.", {"Merchant's Wisdom Note"}, {"Recipe Fragment A", "Recipe Fragment B"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Ah, another Aldrich! I'm afraid the family's financial situation is... complicated.", {}, {"Merchant's Wisdom Note"}, {}, {}, false, false, true, false},
                {"Theodore spent everything on his occult research.\nI'm trapped in the moment I realized we were ruined.", {}, {"Merchant's Wisdom Note"}, {"Merchant's Wisdom Note"}, {}, false, false, true, true},
            }}},

            {"Cartographer", {{ // Source: Study
                // Subsequent Meetings (Has Cartographer's Maps Note)
                {"Each room has three states representing different aspects of human experience.\nTheodore thought he could catalog and control memory itself, but memory is a living thing.", {"Cartographer's Maps Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Greetings, fellow explorer!\nI've been mapping this house's supernatural properties for... well, time has no meaning here.", {}, {"Cartographer's Maps Note"}, {}, {}, false, false, true, false},
                {"Every room state, every transformation, every hidden connection.\nThe house follows patterns, even in its madness.", {}, {"Cartographer's Maps Note"}, {"Cartographer's Maps Note"}, {}, false, false, true, true},
            }}},

            {"Astronomer", {{ // Source: Observatory
                // Subsequent Meetings (Has Astronomer's Vision Note)
                {"Theodore asked me to calculate the perfect moment for his ritual.\nI gave him exact times, precise alignments... but I should have calculated the cost, not just the opportunity.", {"Astronomer's Vision Note"}, {}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"The stars align in mysterious ways, traveler.", {}, {"Astronomer's Vision Note"}, {}, {}, false, false, true, false},
                {"I calculated the perfect astrological timing for Theodore's ritual... but I failed to foresee its consequences.", {}, {"Astronomer's Vision Note"}, {"Astronomer's Vision Note"}, {}, false, false, true, true},
            }}},

            // ===== TRANSFORMATION ITEMS =====

            {"Family Portrait", {{ // Controls North Room → Garden (from Vault)
                {"A photo of happier times before the curse.\nCarrying it draws you toward natural beauty and growth.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room stirs with longing for simpler days.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Family Portrait", {}, {}, {"Family Portrait"}, {}, false, true, false, true},
            }}},

            {"Family Portrait Return", {{ // Can only return to Vault
                {"You place the portrait back on the safe with reverence.", {"Family Portrait"}, {}, {}, {}, false, false, false, false},
                {"The North Room settles, its yearning quieted.", {"Family Portrait"}, {}, {}, {}, true, false, false, false},
                {"Put Back Family Portrait", {"Family Portrait"}, {}, {}, {"Family Portrait"}, false, true, false, true},
            }}},

            {"Broken Clockwork", {{ // Controls North Room → Armory (from Treasury)
                {"Gears and springs from an old timepiece.\nCarrying it awakens thoughts of honor and duty.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room resonates with martial precision.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Broken Clockwork", {}, {}, {"Broken Clockwork"}, {}, false, true, false, true},
            }}},

            {"Broken Clockwork Return", {{ // Can only return to Treasury
                {"You return the clockwork to its resting place.", {"Broken Clockwork"}, {}, {}, {}, false, false, false, false},
                {"The North Room's military bearing fades.", {"Broken Clockwork"}, {}, {}, {}, true, false, false, false},
                {"Put Back Broken Clockwork", {"Broken Clockwork"}, {}, {}, {"Broken Clockwork"}, false, true, false, true},
            }}},

            {"Love Letter", {{ // Controls East Room → Kitchen (from Library)
                {"Theodore's proposal letter filled with domestic dreams.\nCarrying it awakens hunger for family comfort.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room warms with thoughts of gathering.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Love Letter", {}, {}, {"Love Letter"}, {}, false, true, false, true},
            }}},

            {"Love Letter Return", {{ // Can only return to Library
                {"You place the letter back among the scholarly works.", {"Love Letter"}, {}, {}, {}, false, false, false, false},
                {"The East Room's domestic warmth dissipates.", {"Love Letter"}, {}, {}, {}, true, false, false, false},
                {"Put Back Love Letter", {"Love Letter"}, {}, {}, {"Love Letter"}, false, true, false, true},
            }}},

            {"Star Chart", {{ // Controls East Room → Lab (from Portal Chamber)
                {"Astronomical maps covered in alchemical symbols.\nCarrying it stirs scientific curiosity.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room hums with experimental energy.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Star Chart", {}, {}, {"Star Chart"}, {}, false, true, false, true},
            }}},

            {"Star Chart Return", {{ // Can only return to Portal Chamber
                {"You return the chart to the mystical apparatus.", {"Star Chart"}, {}, {}, {}, false, false, false, false},
                {"The East Room's scientific fervor cools.", {"Star Chart"}, {}, {}, {}, true, false, false, false},
                {"Put Back Star Chart", {"Star Chart"}, {}, {}, {"Star Chart"}, false, true, false, true},
            }}},

            {"Recipe Box", {{ // Controls South Room → Bedroom (from Library)
                {"A wooden box that once held family recipes.\nCarrying it awakens longing for personal comfort.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room softens with intimate memories.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Recipe Box", {}, {}, {"Recipe Box"}, {}, false, true, false, true},
            }}},

            {"Recipe Box Return", {{ // Can only return to Library
                {"You return the recipe box to its scholarly home.", {"Recipe Box"}, {}, {}, {}, false, false, false, false},
                {"The South Room's personal warmth recedes.", {"Recipe Box"}, {}, {}, {}, true, false, false, false},
                {"Put Back Recipe Box", {"Recipe Box"}, {}, {}, {"Recipe Box"}, false, true, false, true},
            }}},

            {"Ritual Candle", {{ // Controls South Room → Study (from Portal Chamber)
                {"Burns with silver flame from Theodore's ritual.\nCarrying it awakens scholarly investigation.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room flickers with research ambition.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Ritual Candle", {}, {}, {"Ritual Candle"}, {}, false, true, false, true},
            }}},

            {"Ritual Candle Return", {{ // Can only return to Portal Chamber
                {"You return the candle to the ritual altar.", {"Ritual Candle"}, {}, {}, {}, false, false, false, false},
                {"The South Room's scholarly fire dims.", {"Ritual Candle"}, {}, {}, {}, true, false, false, false},
                {"Put Back Ritual Candle", {"Ritual Candle"}, {}, {}, {"Ritual Candle"}, false, true, false, true},
            }}},

            {"Old Journal", {{ // Controls West Room → Workshop (from Vault)
                {"Personal diary filled with daily life and practical concerns.\nCarrying it awakens the urge to create and build.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room stirs with creative potential.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Old Journal", {}, {}, {"Old Journal"}, {}, false, true, false, true},
            }}},

            {"Old Journal Return", {{ // Can only return to Vault
                {"You return the journal to its secure resting place.", {"Old Journal"}, {}, {}, {}, false, false, false, false},
                {"The West Room's creative energy subsides.", {"Old Journal"}, {}, {}, {}, true, false, false, false},
                {"Put Back Old Journal", {"Old Journal"}, {}, {}, {"Old Journal"}, false, true, false, true},
            }}},

            {"Research Notes", {{ // Controls West Room → Observatory (from Treasury)
                {"Cordelia's research into the family curse and astronomical phenomena.\nCarrying it stirs transcendent ambitions.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room reaches toward celestial mysteries.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Research Notes", {}, {}, {"Research Notes", "Moon Symbol Seen"}, {}, false, true, false, true},
            }}},

            {"Research Notes Return", {{ // Can only return to Treasury
                {"You organize the notes back among the valuables.", {"Research Notes"}, {}, {}, {}, false, false, false, false},
                {"The West Room's starward gaze lowers.", {"Research Notes"}, {}, {}, {}, true, false, false, false},
                {"Put Back Research Notes", {"Research Notes"}, {}, {}, {"Research Notes"}, false, true, false, true},
            }}},

            // ===== QUEST ITEMS =====

            {"Encrypted Note", {{ // Scholar (Library) - Given during first dialogue
                {"The Scholar's encrypted research, covered in mystical symbols.\n'Only the three sacred herbs can decode this cipher.'", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Moonbell Herb", {{ // Source: Garden (when North Room is Garden)
                {"A silvery herb that chimes with memory.\nThe Scholar seeks this for his cipher.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Moonbell Herb", {}, {}, {"Moonbell Herb"}, {}, false, true, false, true},
            }}},

            {"Fireroot", {{ // Source: Lab (when East Room is Lab)
                {"A red herb that burns cold.\nEssential for the Scholar's cipher.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Fireroot", {}, {}, {"Fireroot"}, {}, false, true, false, true},
            }}},

            {"Stargrass", {{ // Source: Observatory (when West Room is Observatory)
                {"An ethereal herb that glows with starlight.\nThe final component for the Scholar's ritual.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Stargrass", {}, {}, {"Stargrass"}, {}, false, true, false, true},
            }}},

            {"Recipe Fragment A", {{ // Source: Bedroom (when South Room is Bedroom)
                {"Half of grandmother's cookie recipe.\nThe torn edge suggests its partner waits elsewhere.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Recipe Fragment A", {}, {}, {"Recipe Fragment A"}, {}, false, true, false, true},
            }}},

            {"Memory Crystal", {{ // Appears in Study during Pattern of Echoes
                {"A crystal formed from overlapping memories.\nIt pulses with potential, waiting to be charged.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Memory Crystal", {}, {}, {"Memory Crystal"}, {}, false, true, false, true},
            }}},

            // ===== NON-PICKUPABLE INTERACTABLES =====

            {"Theodore's Safe", {{ // Vault only
                // With Safe Combination Note
                {"You enter the combination 7-3-9.\nThe safe opens with a satisfying click, revealing treasures within!", {"Safe Combination Note"}, {}, {"Yellow Master Key"}, {}, false, false, false, true},
                // Without combination
                {"A locked safe requiring a three-number combination.\nThe Scholar might know how to open it.", {}, {"Safe Combination Note"}, {}, {}, false, false, false, true},
            }}},

            {"Sun Symbol Stone", {{ // Armory only
                {"Carved into the wall: a radiant sun.\n'In the beginning, there was light.'", {}, {}, {"Sun Symbol Seen"}, {}, false, false, false, true},
            }}},

            {"Moon Symbol Text", {{ // Study only (in Research Notes area)
                {"Written in the research notes: a crescent moon.\n'Preservation follows creation.'", {}, {}, {"Moon Symbol Seen"}, {}, false, false, false, true},
            }}},

            {"Spiral Symbol Chart", {{ // Observatory only
                {"Drawn on star charts: an eternal spiral.\n'All things return, but changed.'", {}, {}, {"Spiral Symbol Seen"}, {}, false, false, false, true},
            }}},

            // ===== OTHER ROOM ITEMS =====

            {"Ceremonial Sword", {{ // Source: Armory
                {"A blade of honor that has never seen battle.\nThe Knight's reflection appears faintly in its polished surface.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Ceremonial Sword", {}, {}, {"Ceremonial Sword"}, {}, false, true, false, true},
            }}},

            {"Ancient Tome", {{ // Source: Library
                {"Theodore's memory research journal, filled with symbols that shift when you're not looking directly at them.\nReading it feels like remembering a dream.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Ancient Tome", {}, {}, {"Ancient Tome"}, {}, false, true, false, true},
            }}},

            {"Fresh Bread", {{ // Source: Kitchen
                {"Still-warm bread from Christmas past.\nTaking a bite fills you with the comfort of family gatherings and the melancholy of knowing they can't last forever.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Fresh Bread", {}, {}, {"Fresh Bread"}, {}, false, true, false, true},
            }}},

            {"Memory Vial", {{ // Source: Lab
                {"Contains swirling silver mist that moves like liquid starlight.\nThe label reads: 'Essence of Christmas Morning, 1967 - Handle with Care.'", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Memory Vial", {}, {}, {"Memory Vial"}, {}, false, true, false, true},
            }}},

            {"Gold Coins", {{ // Source: Treasury
                {"Tarnished currency from another era.\nThey can't buy back lost time or repair broken relationships.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Gold Coins", {}, {}, {"Gold Coins"}, {}, false, true, false, true},
            }}},

            {"Personal Diary", {{ // Source: Bedroom
                {"Young Cordelia's journal, documenting her first encounters with the supernatural.\nThe final entry reads: 'The house remembers what we try to forget, but forgetting might be more merciful than remembering forever.'", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Personal Diary", {}, {}, {"Personal Diary"}, {}, false, true, false, true},
            }}},

            {"House Maps", {{ // Source: Study
                {"Detailed blueprints showing every room configuration, hidden passage, and secret compartment.\nRed X marks indicate where temporal energy concentrates.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained House Maps", {}, {}, {"House Maps"}, {}, false, true, false, true},
            }}},

            {"Family Tree", {{ // Source: Vault
                {"A genealogical chart showing how the curse has affected each generation of Aldrichs.\nYour name appears at the bottom, with a question mark for your fate.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Family Tree", {}, {}, {"Family Tree"}, {}, false, true, false, true},
            }}},

            {"Hammer", {{ // Source: Workshop
                {"A well-used carpenter's hammer, worn smooth by years of honest work.\nIt represents the satisfaction of building something lasting through effort rather than magic.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Hammer", {}, {}, {"Hammer"}, {}, false, true, false, true},
            }}},

            {"Telescope", {{ // Source: Observatory
                {"Points not at the night sky, but at temporal possibilities.\nThrough it, you can observe potential room transformations and glimpse the consequences of different choices.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Telescope", {}, {}, {"Telescope"}, {}, false, true, false, true},
            }}},

            {"Crystal Formations", {{ // Source: Portal Chamber (non-pickupable)
                {"Crystallized memories jutting from the walls like frozen lightning.\nEach formation contains someone's trapped moment - you can hear faint echoes of laughter, weeping, and whispered words of love.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Watering Can", {{ // Source: Garden
                {"Contains water that never seems to empty or evaporate.\nThe Gardener uses it to tend memories as if they were flowers.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Watering Can", {}, {}, {"Watering Can"}, {}, false, true, false, true},
            }}},

            {"Laboratory Notes", {{ // Source: Lab
                {"Theodore's experimental records, documenting his attempts to crystallize human consciousness.\nThe later entries show increasing desperation and decreasing sanity.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Laboratory Notes", {}, {}, {"Laboratory Notes"}, {}, false, true, false, true},
            }}},

            {"Jewelry Box", {{ // Source: Treasury
                {"Contains engagement rings, wedding bands, and heirloom pieces -\nsymbols of love that outlasted the relationships they celebrated.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Jewelry Box", {}, {}, {"Jewelry Box"}, {}, false, true, false, true},
            }}},

            {"Hand Mirror", {{ // Source: Bedroom
                {"Reflects not your current appearance, but glimpses of who you might become.\nSometimes you see yourself older, sometimes younger, sometimes not yourself at all.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Hand Mirror", {}, {}, {"Hand Mirror"}, {}, false, true, false, true},
            }}},

            {"Magnifying Glass", {{ // Source: Study
                {"Reveals hidden text and symbols throughout the house.\nThrough it, you can see the ghostly outlines of items and people from other time periods overlapping the present.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Magnifying Glass", {}, {}, {"Magnifying Glass"}, {}, false, true, false, true},
            }}},

            {"Theodore's Final Journal", {{ // Source: Vault
                {"His last entries, written as the ritual began.\nThey reveal a man who realized too late that he was trapping rather than preserving, but was too proud to stop.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Theodore's Final Journal", {}, {}, {"Theodore's Final Journal"}, {}, false, true, false, true},
            }}},

            {"Tool Box", {{ // Source: Workshop
                {"Contains various implements for building, fixing, and creating.\nEach tool represents a different approach to solving problems through practical action.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Tool Box", {}, {}, {"Tool Box"}, {}, false, true, false, true},
            }}},

            {"Summoning Candles", {{ // Source: Portal Chamber (non-pickupable)
                {"Black candles that burn with silver flames.\nThey were lit the night of Theodore's ritual and have never gone out, sustained by the temporal energy they helped unleash.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Recipe Fragment B", {{ // Given by Inventor in trade
                {"The other half of the recipe: '...1 tsp vanilla, bake at 350°F until golden. Makes enough to feed the whole family.'\nA note adds: 'Love is the secret ingredient.'", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Safe Combination Note", {{ // Given by Scholar after herb quest
                {"The Scholar's decoded cipher reveals: 'The combination is 7-3-9.\nTheodore hid his greatest secret in the Vault.'", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Silver Key", {{ // Given by Merchant
                {"A tarnished key that unlocks additional vault treasures.\nEngraved with: 'Some treasures are worth more than gold.'", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Charged Memory Crystal", {{ // Given by Dreamer
                {"The crystal now blazes with inner light, containing the essence of pure possibility.\nIt whispers of futures yet unwritten.", {}, {}, {}, {}, false, false, false, true},
            }}},

            // ===== RETURN INTERACTIONS FOR ROOM ITEMS =====

            {"Ancient Tome Return", {{ // Can only return to Library
                {"You place the tome back among the scholarly works.", {"Ancient Tome"}, {}, {}, {}, false, false, false, false},
                {"The ancient knowledge settles back into its rightful place.", {"Ancient Tome"}, {}, {}, {}, false, false, false, false},
                {"Put Back Ancient Tome", {"Ancient Tome"}, {}, {}, {"Ancient Tome"}, false, true, false, true},
            }}},

            {"Fresh Bread Return", {{ // Can only return to Kitchen
                {"You return the bread to the counter where Christmas never ends.", {"Fresh Bread"}, {}, {}, {}, false, false, false, false},
                {"The warmth of family gatherings settles back into place.", {"Fresh Bread"}, {}, {}, {}, false, false, false, false},
                {"Put Back Fresh Bread", {"Fresh Bread"}, {}, {}, {"Fresh Bread"}, false, true, false, true},
            }}},

            {"Memory Vial Return", {{ // Can only return to Lab
                {"You carefully place the vial back on the laboratory shelf.", {"Memory Vial"}, {}, {}, {}, false, false, false, false},
                {"The crystallized memories swirl back into their experimental rest.", {"Memory Vial"}, {}, {}, {}, false, false, false, false},
                {"Put Back Memory Vial", {"Memory Vial"}, {}, {}, {"Memory Vial"}, false, true, false, true},
            }}},

            {"Gold Coins Return", {{ // Can only return to Treasury
                {"You return the coins to their chest among the other valuables.", {"Gold Coins"}, {}, {}, {}, false, false, false, false},
                {"The tarnished wealth settles back into meaningless accumulation.", {"Gold Coins"}, {}, {}, {}, false, false, false, false},
                {"Put Back Gold Coins", {"Gold Coins"}, {}, {}, {"Gold Coins"}, false, true, false, true},
            }}},

            {"Personal Diary Return", {{ // Can only return to Bedroom
                {"You close the diary and place it back on the nightstand.", {"Personal Diary"}, {}, {}, {}, false, false, false, false},
                {"Young Cordelia's secrets return to their intimate sanctuary.", {"Personal Diary"}, {}, {}, {}, false, false, false, false},
                {"Put Back Personal Diary", {"Personal Diary"}, {}, {}, {"Personal Diary"}, false, true, false, true},
            }}},

            {"House Maps Return", {{ // Can only return to Study
                {"You organize the maps back on the research desk.", {"House Maps"}, {}, {}, {}, false, false, false, false},
                {"The cartographic knowledge returns to its academic home.", {"House Maps"}, {}, {}, {}, false, false, false, false},
                {"Put Back House Maps", {"House Maps"}, {}, {}, {"House Maps"}, false, true, false, true},
            }}},

            {"Family Tree Return", {{ // Can only return to Vault
                {"You place the genealogical chart back among the family secrets.", {"Family Tree"}, {}, {}, {}, false, false, false, false},
                {"The record of the Aldrich curse returns to its guarded rest.", {"Family Tree"}, {}, {}, {}, false, false, false, false},
                {"Put Back Family Tree", {"Family Tree"}, {}, {}, {"Family Tree"}, false, true, false, true},
            }}},

            {"Hammer Return", {{ // Can only return to Workshop
                {"You set the hammer back on the workbench where it belongs.", {"Hammer"}, {}, {}, {}, false, false, false, false},
                {"The tool of honest creation returns to its practical purpose.", {"Hammer"}, {}, {}, {}, false, false, false, false},
                {"Put Back Hammer", {"Hammer"}, {}, {}, {"Hammer"}, false, true, false, true},
            }}},

            {"Telescope Return", {{ // Can only return to Observatory
                {"You carefully return the telescope to its celestial mount.", {"Telescope"}, {}, {}, {}, false, false, false, false},
                {"The instrument of transcendent vision returns to its starward gaze.", {"Telescope"}, {}, {}, {}, false, false, false, false},
                {"Put Back Telescope", {"Telescope"}, {}, {}, {"Telescope"}, false, true, false, true},
            }}},

            {"Watering Can Return", {{ // Can only return to Garden
                {"You place the watering can back among the memory flowers.", {"Watering Can"}, {}, {}, {}, false, false, false, false},
                {"The tender of memories returns to its eternal gardening.", {"Watering Can"}, {}, {}, {}, false, false, false, false},
                {"Put Back Watering Can", {"Watering Can"}, {}, {}, {"Watering Can"}, false, true, false, true},
            }}},

            {"Laboratory Notes Return", {{ // Can only return to Lab
                {"You organize the experimental notes back on the laboratory table.", {"Laboratory Notes"}, {}, {}, {}, false, false, false, false},
                {"Theodore's dangerous research returns to its scientific rest.", {"Laboratory Notes"}, {}, {}, {}, false, false, false, false},
                {"Put Back Laboratory Notes", {"Laboratory Notes"}, {}, {}, {"Laboratory Notes"}, false, true, false, true},
            }}},

            {"Jewelry Box Return", {{ // Can only return to Treasury
                {"You place the jewelry box back among the other precious things.", {"Jewelry Box"}, {}, {}, {}, false, false, false, false},
                {"The symbols of love return to their valuable but empty collection.", {"Jewelry Box"}, {}, {}, {}, false, false, false, false},
                {"Put Back Jewelry Box", {"Jewelry Box"}, {}, {}, {"Jewelry Box"}, false, true, false, true},
            }}},

            {"Hand Mirror Return", {{ // Can only return to Bedroom
                {"You place the mirror back on the dresser with care.", {"Hand Mirror"}, {}, {}, {}, false, false, false, false},
                {"The glimpses of possible futures return to their intimate reflection.", {"Hand Mirror"}, {}, {}, {}, false, false, false, false},
                {"Put Back Hand Mirror", {"Hand Mirror"}, {}, {}, {"Hand Mirror"}, false, true, false, true},
            }}},

            {"Magnifying Glass Return", {{ // Can only return to Study
                {"You set the magnifying glass back among the research tools.", {"Magnifying Glass"}, {}, {}, {}, false, false, false, false},
                {"The revealer of hidden truths returns to its scholarly purpose.", {"Magnifying Glass"}, {}, {}, {}, false, false, false, false},
                {"Put Back Magnifying Glass", {"Magnifying Glass"}, {}, {}, {"Magnifying Glass"}, false, true, false, true},
            }}},

            {"Theodore's Final Journal Return", {{ // Can only return to Vault
                {"You place the journal back among Theodore's most guarded secrets.", {"Theodore's Final Journal"}, {}, {}, {}, false, false, false, false},
                {"The record of ultimate failure returns to its shameful hiding.", {"Theodore's Final Journal"}, {}, {}, {}, false, false, false, false},
                {"Put Back Theodore's Final Journal", {"Theodore's Final Journal"}, {}, {}, {"Theodore's Final Journal"}, false, true, false, true},
            }}},

            {"Tool Box Return", {{ // Can only return to Workshop
                {"You return the tool box to its place among the other implements.", {"Tool Box"}, {}, {}, {}, false, false, false, false},
                {"The collection of practical solutions returns to its creative home.", {"Tool Box"}, {}, {}, {}, false, false, false, false},
                {"Put Back Tool Box", {"Tool Box"}, {}, {}, {"Tool Box"}, false, true, false, true},
            }}},

            {"Ceremonial Sword Return", {{ // Can only return to Armory
                {"You return the sword to its place of honor on the weapon rack.", {"Ceremonial Sword"}, {}, {}, {}, false, false, false, false},
                {"The blade of untested valor returns to its ceremonial rest.", {"Ceremonial Sword"}, {}, {}, {}, false, false, false, false},
                {"Put Back Ceremonial Sword", {"Ceremonial Sword"}, {}, {}, {"Ceremonial Sword"}, false, true, false, true},
            }}},

            // ===== FINAL VICTORY =====

            {"Final Pedestals", {{ // Courtyard - when all 4 Master Keys held
                {"With all four Master Keys in hand, the fountain begins to glow.\nFour pedestals rise from the water, each awaiting its destined key.", {"Red Master Key", "Green Master Key", "Yellow Master Key", "Purple Master Key"}, {}, {}, {}, false, false, false, false},
                {"Knowledge flows through the house's foundation.", {"Red Master Key", "Green Master Key", "Yellow Master Key", "Purple Master Key"}, {}, {}, {"Red Master Key"}, false, false, false, false},
                {"Warmth spreads from remembered family gatherings.", {"Green Master Key", "Yellow Master Key", "Purple Master Key"}, {}, {}, {"Green Master Key"}, false, false, false, false},
                {"Truth illuminates the house's dark corners.", {"Yellow Master Key", "Purple Master Key"}, {}, {}, {"Yellow Master Key"}, false, false, false, false},
                {"The final key! The house's temporal bonds shatter like glass.", {"Purple Master Key"}, {}, {}, {"Purple Master Key"}, true, false, false, false},
                {"The exit door unseals with a sound like exhaling.\nThe house's memory loops are broken, and you are free to leave.", {}, {}, {}, {}, false, false, false, true},
            }}},
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
            bool m_breakAfterStep = false;
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


                int interactionStep = 0;
                while (!TrySetInteractionStep(context, interactionStep))
                {
                    interactionStep++;
                    if (interactionStep == m_interaction.dialougeVector.size())
                    {
                        BREAK_MSG("No valid interaction steps for this interaction");
                        stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                        return;
                    }
                }
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
                    else
                    {
                        if (m_breakAfterStep)
                        {
                            stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                            return; // we are finished with the interaction.
                        }
                        int interactionStep = m_interactionStep + 1;
                        while (!TrySetInteractionStep(context, interactionStep))
                        {
                            interactionStep++;
                            if (interactionStep == m_interaction.dialougeVector.size())
                            {
                                stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                                return;
                            }
                        }
                        m_continueDialogueLabel->SetVisible(false);
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

            bool TrySetInteractionStep(GameContext& context, int stepIndex)
            {
                Core::GameData& gameDate = context.GetGameData();
                Inventory& inventory = context.GetInventory();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                DialogueStep step = m_interaction.dialougeVector[stepIndex];
                // check step conditions
                for (auto& item : step.conditionContainsItemVector)
                {
                    if(std::find(inventory.begin(), inventory.end(), item) == inventory.end())
                    {
                        return false;
                    }
                }
                for (auto& item : step.conditionExcludeItemVector)
                {
                    if(std::find(inventory.begin(), inventory.end(), item) != inventory.end())
                    {
                        return false;
                    }
                }

                m_currentString = step.dialogue;
                m_dialogueSrolling = step.animateText;
                m_breakAfterStep = step.exitInteraction;
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
                    entt::registry& registry = context.GetRegistry();
                    auto& interactableComponent = registry.get<Component::Interactable>(m_interactingEntity);
                    if (interactableComponent.canBeReturned)
                    {
                        // TODO Change the items sprite (Toggle the sprite index)
                        std::string& interactionId = interactableComponent.interactionId;
                        const std::string suffix = " Return";
                        
                        // Check if string ends with " returns"
                        if (interactionId.length() >= suffix.length() && 
                            interactionId.substr(interactionId.length() - suffix.length()) == suffix)
                        {
                            // Remove the suffix
                            interactionId = interactionId.substr(0, interactionId.length() - suffix.length());
                        }
                        else
                        {
                            // Add the suffix
                            interactionId = interactionId + suffix;
                        }
                    }
                    else
                    {
                        gameObjectManager.DestroyGameObject(context, m_interactingEntity);
                    }                    
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

                return true;
            }
        };
    }
}
