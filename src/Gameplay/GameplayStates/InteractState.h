#pragma once

#include <cstdlib>
#include <algorithm>
#include <format>
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
                // After receiving Ancient Tome and giving Red Crystal Key
                {"Ah, my precious Ancient Tome! Years of research finally returned to me.\nThis knowledge belongs in my library, where it can illuminate minds for eternity.", {"Red Crystal Key"}, {}, {}, {}, false, false, true, true},
                // Has Ancient Tome to trade
                {"Perfect! My lost research tome! This contains decades of study into the house's mysteries.", {"Scholar's Memory Note", "Ancient Tome"}, {"Red Crystal Key"}, {}, {}, false, false, true, false},
                {"In exchange for returning my life's work, I offer you this Crystal Key of Knowledge.\nMay it unlock the secrets you seek.", {"Scholar's Memory Note", "Ancient Tome"}, {"Red Crystal Key"}, {"Red Crystal Key"}, {"Ancient Tome"}, false, false, true, false},
                {"Obtained Red Crystal Key", {"Scholar's Memory Note", "Red Crystal Key"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Ancient Tome
                {"My Ancient Tome is still missing - it contains all my research into this house's curse.\nPerhaps the Gardener has seen it? She tends to collect beautiful things in her garden.", {"Scholar's Memory Note"}, {"Ancient Tome"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my library, fellow seeker of knowledge.\nI've been trapped here since 1943, studying the curse that binds us all.", {}, {"Scholar's Memory Note"}, {}, {}, false, false, true, false},
                {"I had nearly solved the mystery when my research was scattered.\nIf you could find my Ancient Tome, I would gladly share a Crystal Key with you.", {}, {"Scholar's Memory Note"}, {"Scholar's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Gardener", {{ // Source: Garden (when Love Letter held)
                // After receiving Rose and giving Ancient Tome
                {"Oh, my beautiful rose! It belongs here among the other flowers, where love once bloomed eternal.\nThis garden is complete again.", {"Ancient Tome"}, {}, {}, {}, false, false, true, true},
                // Has Rose to trade for Ancient Tome
                {"A lovely rose! Yes, I do have the Scholar's tome, but this garden feels incomplete without its most beautiful flower.", {"Gardener's Memory Note", "Rose"}, {"Ancient Tome"}, {}, {}, false, false, true, false},
                {"I'll gladly trade this dusty old book for something that belongs in a garden of eternal spring.", {"Gardener's Memory Note", "Rose"}, {"Ancient Tome"}, {"Ancient Tome"}, {"Rose"}, false, false, true, false},
                {"Obtained Ancient Tome", {"Gardener's Memory Note", "Ancient Tome"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Rose
                {"Welcome to my garden where love once bloomed.\nI found the Scholar's tome among my flowers, but I won't trade it for just anything.", {"Gardener's Memory Note"}, {"Rose"}, {}, {}, false, false, true, false},
                {"Bring me a perfect rose - I've heard the Inventor has one in his workshop.\nA garden needs its most precious flower.", {"Gardener's Memory Note"}, {"Rose"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome, dear visitor, to my sanctuary of eternal spring.\nI tend to memories as if they were flowers, nurturing what should grow and bloom.", {}, {"Gardener's Memory Note"}, {}, {}, false, false, true, false},
                {"Love requires patience and care, just like any garden.\nSome things belong together - a rose belongs among flowers, not gathering dust in workshops.", {}, {"Gardener's Memory Note"}, {"Gardener's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Cook", {{ // Source: Kitchen
                // After receiving Fresh Bread and giving Green Crystal Key
                {"My warm, fresh bread! The kitchen feels like home again with the scent of baking.\nThis Crystal Key has been keeping my recipes company - please, take it.", {"Green Crystal Key"}, {}, {}, {}, false, false, true, true},
                // Has Fresh Bread to trade
                {"Oh, wonderful! Fresh bread, still warm from the oven!\nThe heart of any kitchen is the bread that feeds the family.", {"Cook's Memory Note", "Fresh Bread"}, {"Green Crystal Key"}, {}, {}, false, false, true, false},
                {"For bringing warmth back to my kitchen, I'll give you this Crystal Key I've been safeguarding.", {"Cook's Memory Note", "Fresh Bread"}, {"Green Crystal Key"}, {"Green Crystal Key"}, {"Fresh Bread"}, false, false, true, false},
                {"Obtained Green Crystal Key", {"Cook's Memory Note", "Green Crystal Key"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Fresh Bread
                {"My kitchen feels so empty without the warmth of fresh bread.\nI believe the Inventor has been keeping some - perhaps as sustenance for his work?", {"Cook's Memory Note"}, {"Fresh Bread"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my kitchen, dear!\nI've been preparing the same Christmas feast for decades,\nwaiting for family to gather.", {}, {"Cook's Memory Note"}, {}, {}, false, false, true, false},
                {"A kitchen without fresh bread is like a hearth without fire.\nBring me the bread that belongs here, and I'll share something precious in return.", {}, {"Cook's Memory Note"}, {"Cook's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Inventor", {{ // Source: Workshop (when Hammer held)
                // After receiving Tool Box and giving Fresh Bread
                {"My tool box! Every craftsman needs his complete set of implements.\nThis bread was keeping me fed during long work sessions, but you've earned it.", {"Fresh Bread"}, {}, {}, {}, false, false, true, true},
                // Has Tool Box to trade
                {"Ah, my missing tool box! I've been making do with individual tools, but a craftsman needs his complete kit.", {"Inventor's Memory Note", "Tool Box"}, {"Fresh Bread"}, {}, {}, false, false, true, false},
                {"I've been saving this fresh bread for sustenance during long projects.\nA fair trade - tools for nourishment!", {"Inventor's Memory Note", "Tool Box"}, {"Fresh Bread"}, {"Fresh Bread"}, {"Tool Box"}, false, false, true, false},
                {"Obtained Fresh Bread", {"Inventor's Memory Note", "Fresh Bread"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Tool Box
                {"My workshop isn't complete without my full set of tools.\nI think the Researcher borrowed my tool box - scholars always need instruments for their work.", {"Inventor's Memory Note"}, {"Tool Box"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my workshop! I've been trying to build our way out of this cursed house for decades.", {}, {"Inventor's Memory Note"}, {}, {}, false, false, true, false},
                {"Every problem has a practical solution if you have the right tools.\nBring me my complete tool box, and I'll trade you something nourishing in return.", {}, {"Inventor's Memory Note"}, {"Inventor's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Dreamer", {{ // Source: Bedroom
                // After receiving Personal Diary and giving Yellow Crystal Key
                {"My diary... full of dreams and visions of freedom.\nThis Crystal Key has been glowing in my dreams - it belongs with someone who seeks truth.", {"Yellow Crystal Key"}, {}, {}, {}, false, false, true, true},
                // Has Personal Diary to trade
                {"My personal diary! I've been dreaming of its return...\nIt contains all my visions and prophetic dreams about this house.", {"Dreamer's Memory Note", "Personal Diary"}, {"Yellow Crystal Key"}, {}, {}, false, false, true, false},
                {"For reuniting me with my dreams and memories, I offer you this Crystal Key of Visions.", {"Dreamer's Memory Note", "Personal Diary"}, {"Yellow Crystal Key"}, {"Yellow Crystal Key"}, {"Personal Diary"}, false, false, true, false},
                {"Obtained Yellow Crystal Key", {"Dreamer's Memory Note", "Yellow Crystal Key"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Personal Diary
                {"I dream of my lost diary... it contains visions that could help us all escape.\nThe Researcher has it - she studies everything, even personal dreams.", {"Dreamer's Memory Note"}, {"Personal Diary"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Mmm... I dream of escape, but the dreams never end...\nI exist in the space between sleeping and waking, seeing glimpses of what could be.", {}, {"Dreamer's Memory Note"}, {}, {}, false, false, true, false},
                {"In my dreams, I see rooms flowing like water, changing with the items we carry...\nFind my diary, and I'll share a key that unlocks more than just doors.", {}, {"Dreamer's Memory Note"}, {"Dreamer's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Astronomer", {{ // Source: Observatory (when Star Chart held)
                // After receiving Telescope and giving Personal Diary
                {"My precious telescope! Now I can properly observe the celestial patterns that govern this house.\nThis diary contains earthly dreams, but the stars hold greater truths.", {"Personal Diary"}, {}, {}, {}, false, false, true, true},
                // Has Telescope to trade
                {"Magnificent! My telescope for studying the stars and their influence on temporal anomalies!", {"Astronomer's Memory Note", "Telescope"}, {"Personal Diary"}, {}, {}, false, false, true, false},
                {"I've been analyzing the Dreamer's diary for cosmic patterns, but I need my telescope more.\nA fair exchange - earthly dreams for celestial observations.", {"Astronomer's Memory Note", "Telescope"}, {"Personal Diary"}, {"Personal Diary"}, {"Telescope"}, false, false, true, false},
                {"Obtained Personal Diary", {"Astronomer's Memory Note", "Personal Diary"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Telescope
                {"I've been calculating the house's temporal distortions, but I need my telescope for proper astronomical observations.\nI believe it's in the Scholar's library - instruments of learning belong together.", {"Astronomer's Memory Note"}, {"Telescope"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Greetings, earthbound traveler! I am Professor Orion Starweaver, astronomer and calculator of celestial influences.", {}, {"Astronomer's Memory Note"}, {}, {}, false, false, true, false},
                {"I've been studying how the stars affect this house's temporal anomalies.\nBring me my telescope, and I'll trade you this diary I've been analyzing for cosmic patterns.", {}, {"Astronomer's Memory Note"}, {"Astronomer's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Merchant", {{ // Source: Treasury
                // After receiving Gold Coins and giving Blue Crystal Key
                {"Ah, my precious gold! These coins represent years of careful investment and trade.\nThis Crystal Key is the most valuable thing I possess - a fair exchange for my life's wealth.", {"Blue Crystal Key"}, {}, {}, {}, false, false, true, true},
                // Has Gold Coins to trade
                {"My gold coins! The foundation of all wealth and prosperity in this house's golden days.", {"Merchant's Memory Note", "Gold Coins"}, {"Blue Crystal Key"}, {}, {}, false, false, true, false},
                {"For returning my fortune, I'll give you this Crystal Key - the most valuable treasure in my collection.", {"Merchant's Memory Note", "Gold Coins"}, {"Blue Crystal Key"}, {"Blue Crystal Key"}, {"Gold Coins"}, false, false, true, false},
                {"Obtained Blue Crystal Key", {"Merchant's Memory Note", "Blue Crystal Key"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Gold Coins
                {"My treasury feels empty without my gold coins - the foundation of all true wealth.\nThe Guardian in the vault has them secured away from common access.", {"Merchant's Memory Note"}, {"Gold Coins"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"Welcome to my treasury!\nI am Augustus Goldstein, and I've spent decades learning what truly has value in this cursed place.", {}, {"Merchant's Memory Note"}, {}, {}, false, false, true, false},
                {"I've learned that some treasures can't be bought or sold, but gold still has its place in the world.\nBring me my coins, and I'll trade you something more valuable than money.", {}, {"Merchant's Memory Note"}, {"Merchant's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Guardian", {{ // Source: Vault (when Key held)
                // After receiving Ancient Seal and giving Gold Coins
                {"The Ancient Seal! This mystical artifact belongs in my vault where its power can be properly contained.\nThese mundane coins are of no concern to me - take them for your mortal pursuits.", {"Gold Coins"}, {}, {}, {}, false, false, true, true},
                // Has Ancient Seal to trade
                {"Excellent! The Ancient Seal of Binding - a powerful artifact that should never be left unprotected.", {"Guardian's Memory Note", "Ancient Seal"}, {"Gold Coins"}, {}, {}, false, false, true, false},
                {"I'll gladly trade mere gold for this relic that could unleash untold dangers if misused.\nIt belongs here, warded and watched.", {"Guardian's Memory Note", "Ancient Seal"}, {"Gold Coins"}, {"Gold Coins"}, {"Ancient Seal"}, false, false, true, false},
                {"Obtained Gold Coins", {"Guardian's Memory Note", "Gold Coins"}, {}, {}, {}, false, false, false, true},
                // Subsequent meetings without Ancient Seal
                {"I am the keeper of this house's most dangerous artifacts and forbidden knowledge.\nThere is an Ancient Seal of Binding that must be secured - I sense it lies unprotected in the bedroom.", {"Guardian's Memory Note"}, {"Ancient Seal"}, {}, {}, false, false, true, true},
                // First Meeting (Default)
                {"I am Master Cedric Voss, Guardian of this vault and protector of the house's most perilous secrets.", {}, {"Guardian's Memory Note"}, {}, {}, false, false, true, false},
                {"Some artifacts are too dangerous to leave unguarded. Bring me the Ancient Seal, and I'll give you gold that holds no mystical corruption.", {}, {"Guardian's Memory Note"}, {"Guardian's Memory Note"}, {}, false, false, true, true},
            }}},

            {"Cordelia", {{ // Courtyard - appears when all 4 Crystal Keys held
                // After all pedestals activated and portal opens
                {"The curse is finally broken, dear Elena. All the trapped souls can rest now.\nThank you for succeeding where I failed. The exit portal awaits - step through and be free.", {"Red Pedestal Active", "Green Pedestal Active", "Yellow Pedestal Active", "Blue Pedestal Active"}, {}, {}, {}, false, false, true, true},
                // When player has all keys but hasn't activated pedestals yet
                {"Elena, my dear great-niece. You've done what I spent my whole life trying to achieve.\nYou have all four Crystal Keys! Now you must activate each pedestal in turn to break Theodore's curse forever.", {"Red Crystal Key", "Green Crystal Key", "Yellow Crystal Key", "Blue Crystal Key"}, {"Red Pedestal Active"}, {}, {}, false, false, true, false},
                {"Place each key in its corresponding pedestal. The red pedestal calls to knowledge, green to creation, yellow to dreams, and blue to protection.\nOnly when all four are activated will the portal to freedom open.", {"Red Crystal Key", "Green Crystal Key", "Yellow Crystal Key", "Blue Crystal Key"}, {"Red Pedestal Active"}, {}, {}, false, false, true, true},
            }}},

            // ===== TRANSFORMATION ITEMS =====

            {"Love Letter", {{ // Controls North Room → Garden (from Bedroom)
                {"A romantic letter filled with promises of eternal love.\nCarrying it awakens thoughts of gardens and blooming romance.", {}, {}, {}, {}, false, false, false, false},
                {"The North Room stirs with romantic longing.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Love Letter", {}, {}, {"Love Letter"}, {}, false, true, false, true},
            }}},

            {"Love Letter Return", {{ // Can only return to Bedroom
                {"You place the letter back on the nightstand where dreams of love belong.", {"Love Letter"}, {}, {}, {}, false, false, false, false},
                {"The North Room's romantic yearning settles back to scholarly calm.", {"Love Letter"}, {}, {}, {}, true, false, false, false},
                {"Put Back Love Letter", {"Love Letter"}, {}, {}, {"Love Letter"}, false, true, false, true},
            }}},

            {"Hammer", {{ // Controls East Room → Workshop (from Treasury)
                {"A well-used hammer that speaks of creation and honest work.\nCarrying it awakens the urge to build and transform.", {}, {}, {}, {}, false, false, false, false},
                {"The East Room resonates with creative potential.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Hammer", {}, {}, {"Hammer"}, {}, false, true, false, true},
            }}},

            {"Hammer Return", {{ // Can only return to Treasury
                {"You return the hammer to its place among the valuables.", {"Hammer"}, {}, {}, {}, false, false, false, false},
                {"The East Room's creative energy settles back to domestic comfort.", {"Hammer"}, {}, {}, {}, true, false, false, false},
                {"Put Back Hammer", {"Hammer"}, {}, {}, {"Hammer"}, false, true, false, true},
            }}},

            {"Star Chart", {{ // Controls South Room → Observatory (from Garden)
                {"Ancient astronomical maps covered in mystical calculations.\nCarrying it awakens thoughts of celestial observations and cosmic mysteries.", {}, {}, {}, {}, false, false, false, false},
                {"The South Room reaches toward the stars and cosmic understanding.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Star Chart", {}, {}, {"Star Chart"}, {}, false, true, false, true},
            }}},

            {"Star Chart Return", {{ // Can only return to Garden
                {"You return the star chart to its secure place among the other valuable documents.", {"Star Chart"}, {}, {}, {}, false, false, false, false},
                {"The South Room's cosmic aspirations settle back to earthly rest.", {"Star Chart"}, {}, {}, {}, true, false, false, false},
                {"Put Back Star Chart", {"Star Chart"}, {}, {}, {"Star Chart"}, false, true, false, true},
            }}},

            {"Ornate Key", {{ // Controls West Room → Vault (from Observatory)
                {"An ornate key that promises to unlock hidden secrets.\nCarrying it awakens thoughts of security and protected treasures.", {}, {}, {}, {}, false, false, false, false},
                {"The West Room shifts toward impenetrable security.", {}, {}, {}, {}, true, false, false, false},
                {"Obtained Key", {}, {}, {"Key"}, {}, false, true, false, true},
            }}},

            {"Ornate Key Return", {{ // Can only return to Observatory
                {"You return the key to its place among the tools and inventions.", {"Key"}, {}, {}, {}, false, false, false, false},
                {"The West Room's protective aura settles back to simple commerce.", {"Key"}, {}, {}, {}, true, false, false, false},
                {"Put Back Key", {"Key"}, {}, {}, {"Key"}, false, true, false, true},
            }}},

            // ===== QUEST ITEMS (Not Returnable) =====

            {"Ancient Tome", {{ // Source: From Gardener
                {"A leather-bound book filled with scholarly research and mystical symbols.\nThe pages seem to shift when you're not looking directly at them.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Ancient Tome", {}, {}, {"Ancient Tome", "Ancient Tome Recipt"}, {}, false, true, false, true},
            }}},

            {"Rose", {{ // Source: Vault
                {"A perfect red rose that never wilts.\nIt smells of eternal spring and promises kept.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Rose", {}, {}, {"Rose", "Rose Recipt"}, {}, false, true, false, true},
            }}},

            {"Fresh Bread", {{ // Source: From Inventory
                {"Still-warm bread that smells of home and family gatherings.\nTaking a bite fills you with comfort and nostalgia.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Fresh Bread", {}, {}, {"Fresh Bread", "Fresh Bread Recipt"}, {}, false, true, false, true},
            }}},

            {"Tool Box", {{ // Source: Obervatory
                {"A weathered box filled with various tools and crafting implements.\nEach tool tells a story of problems solved and things built.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Tool Box", {}, {}, {"Tool Box", "Tool Box Recipt"}, {}, false, true, false, true},
            }}},

            {"Personal Diary", {{ // Source: From Astromoner
                {"A private journal filled with intimate thoughts and dreams.\nThe final entry speaks of hope for escape from endless sleep.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Personal Diary", {}, {}, {"Personal Diary", "Personal Diary Recipt"}, {}, false, true, false, true},
            }}},

            {"Telescope", {{ // Source: Garden
                {"A brass telescope for observing celestial phenomena.\nThrough its lens, you can see the cosmic patterns that influence earthly events.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Telescope", {}, {}, {"Telescope", "Telescope Recipt"}, {}, false, true, false, true},
            }}},
            
            {"Gold Coins", {{ // Source: From Gardian
                {"Tarnished gold coins from another era.\nThey feel heavy with the weight of dreams that money couldn't buy.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Gold Coins", {}, {}, {"Gold Coins", "Gold Coins Recipt"}, {}, false, true, false, true},
            }}},

            {"Ancient Seal", {{ // Source: Workshop
                {"A mystical seal carved with protective wards and binding runes.\nIt pulses with contained power - clearly a dangerous artifact that should be properly secured.", {}, {}, {}, {}, false, false, false, false},
                {"Obtained Ancient Seal", {}, {}, {"Ancient Seal", "Ancient Seal Recipt"}, {}, false, true, false, true},
            }}},

            // ===== NON-PICKUPABLE INTERACTABLES =====

            {"Safe", {{ // Source: Vault (non-pickupable)
                {"An imposing vault safe covered in mystical symbols.\nIt emanates an aura of protection and hidden secrets.", {}, {}, {}, {}, false, false, false, true},
            }}},

            // ===== FINAL VICTORY =====

            {"Red Pedestal Inactive", {{ // Courtyard - appears when all 4 keys held
                {"A crimson pedestal carved with symbols of knowledge and wisdom.\nIt pulses faintly, waiting for the Red Crystal Key to unlock its power.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Red Pedestal Active", {{ // After placing Red Crystal Key
                {"The Red Pedestal blazes with the light of accumulated knowledge.\nThe Scholar's wisdom flows through the crystal, illuminating ancient secrets.", {}, {}, {}, {}, false, false, false, false},
                {"Knowledge flows through the house's foundation like rivers of light.", {}, {}, {}, {}, false, false, false, false},
                {"Activated Red Pedestal", {"Red Crystal Key"}, {"Red Pedestal Active"}, {"Red Pedestal Active"}, {"Red Crystal Key"}, false, false, false, true},
            }}},

            {"Green Pedestal Inactive", {{ // Courtyard - appears when all 4 keys held  
                {"An emerald pedestal carved with symbols of creation and craftsmanship.\nIt hums softly, waiting for the Green Crystal Key to awaken its potential.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Green Pedestal Active", {{ // After placing Green Crystal Key
                {"The Green Pedestal resonates with creative energy and the joy of building.\nThe Inventor's ingenuity flows through the crystal, sparking new possibilities.", {}, {}, {}, {}, false, false, false, false},
                {"Creative energy spreads through the house like growing vines.", {}, {}, {}, {}, false, false, false, false},
                {"Activated Green Pedestal", {"Green Crystal Key"}, {"Green Pedestal Active"}, {"Green Pedestal Active"}, {"Green Crystal Key"}, false, false, false, true},
            }}},

            {"Yellow Pedestal Inactive", {{ // Courtyard - appears when all 4 keys held
                {"A golden pedestal carved with symbols of dreams and visions.\nIt glows softly, waiting for the Yellow Crystal Key to fulfill its destiny.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Yellow Pedestal Active", {{ // After placing Yellow Crystal Key
                {"The Yellow Pedestal shimmers with the light of prophetic dreams.\nThe Dreamer's visions flow through the crystal, revealing glimpses of freedom.", {}, {}, {}, {}, false, false, false, false},
                {"Prophetic energy swirls through the house like golden mist.", {}, {}, {}, {}, false, false, false, false},
                {"Activated Yellow Pedestal", {"Yellow Crystal Key"}, {"Yellow Pedestal Active"}, {"Yellow Pedestal Active"}, {"Yellow Crystal Key"}, false, false, false, true},
            }}},

            {"Blue Pedestal Inactive", {{ // Courtyard - appears when all 4 keys held
                {"A sapphire pedestal carved with symbols of protection and ancient secrets.\nIt emanates quiet power, waiting for the Blue Crystal Key to complete the circle.", {}, {}, {}, {}, false, false, false, true},
            }}},

            {"Blue Pedestal Active", {{ // After placing Blue Crystal Key
                {"The Blue Pedestal radiates protective power and the weight of guarded secrets.\nThe Guardian's wisdom flows through the crystal, unlocking the final barrier.", {}, {}, {}, {}, false, false, false, false},
                {"Protective energy surrounds the house like an ancient shield dissolving.", {}, {}, {}, {}, false, false, false, false},
                {"Activated Blue Pedestal", {"Blue Crystal Key"}, {"Blue Pedestal Active"}, {"Blue Pedestal Active"}, {"Blue Crystal Key"}, false, false, false, true},
            }}},

            {"Entrance Door", {{ // North Room - the door player entered through
                // After entering the house (door becomes one-way)
                {"The heavy oak door you entered through stands firmly shut.\nThe handle turns, but the door refuses to budge - as if the house itself holds it closed.", {}, {}, {}, {}, false, false, false, false},
                {"Strange symbols have appeared along the door frame, glowing faintly with an otherworldly light.\nA whispered voice seems to echo from within the wood: 'None may leave until the four keys sing as one.'", {}, {}, {}, {}, false, false, false, false},
                {"The door remembers your entry, but it has forgotten how to let you leave.\nYou sense this threshold will not open again until the house's curse is broken.", {}, {}, {}, {}, false, false, false, true},
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

                auto* textBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, -50}, glm::vec2{0.5f, 1.0f}, glm::vec2{900, 150}, glm::vec2{0, 0})));
                textBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 1.0f});
                textBackgroundPanel->SetBackgroundColor(DARKGRAY);
                textBackgroundPanel->SetBorderColor(WHITE);
                textBackgroundPanel->SetBorderWidth(2.0f);

                m_dialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 20, 40 }, glm::vec2{ 0, 0 }, "", 18.0f)));
                m_dialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ -20, -20 }, glm::vec2{ 1, 1 }, "Continue", 20.0f)));
                m_continueDialogueLabel->SetAnchorPoint(glm::vec2{1.0f, 1.0f});
                m_continueDialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel->SetVisible(false);

                // Set State variables
                Component::Interactable interactable = registry.get<Component::Interactable>(m_interactingEntity);

                m_interaction = s_interactionMap[interactable.interactionId];

                ASSERT_MSG(m_interaction.dialougeVector.size() > 0, std::format("No interaction set up for {}", interactable.interactionId).c_str());

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
