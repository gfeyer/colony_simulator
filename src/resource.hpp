#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <string>
#include <map>
#include <memory>
#include <SFML/Graphics.hpp>
#include "TGUI/TGUI.hpp"

namespace Resource::Paths{
    // Textures
    constexpr const char* BACKGROUND_4 = "resources/background/Background-4.png";

    // Themes
    constexpr const char* DARK_THEME = "resources/Dark.txt";
}


namespace Resource{

class Manager {
private:
    std::map<std::string, std::unique_ptr<sf::Texture>> textures;
    std::map<std::string, tgui::Theme::Ptr> themes;

private:
    Manager();  // Private constructor
    ~Manager();

    void loadTexture(const char* path);
    void loadTheme(const char* path);

public:
    // Get the singleton instance
    static Manager& getInstance();

    const sf::Texture& getTexture(const char* path) const;
    const tgui::Theme::Ptr getTheme(const char* path) const;
};

}


#endif