#pragma once

#include "platform/platform.hpp"
#if 0
class TiledMapLoadingSystem final : public TriggerSystem {
public:

    UniquePtr<Loader> operator () (Scene &) const final;
};
#endif
UniquePtr<Preloader> make_tiled_map_preloader
    (const char * filename, Platform::ForLoaders &);
