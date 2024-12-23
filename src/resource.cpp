#include "resource.hpp"

#include <memory>
#include "util/logger.hpp"

namespace Resource{

Manager::Manager() {
    log_info << "Loading resources";
    loadTexture(Paths::BACKGROUND_4);
    loadTheme(Paths::DARK_THEME);
}

Manager::~Manager() {
    log_info << "Unloading resources";
}

void Manager::loadTexture(const char *path)
{
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(path)) {
        log_err << "Failed to load texture: " << path << std::endl;
    }
    textures.insert_or_assign(path, std::move(texture));
}

void Manager::loadTheme(const char *path)
{
    auto theme = tgui::Theme::create(path);
    themes.insert_or_assign(path, theme);
}

Manager& Manager::getInstance() {
    static Manager instance; // Singleton instance but allocate once on stack instead of heap, thread-safe
    return instance;
}

const sf::Texture& Manager::getTexture(const char *path) const
{
    return *textures.at(path);
}

const tgui::Theme::Ptr Manager::getTheme(const char *path) const
{
    return themes.at(path);
}


}