#pragma once
#include "ResourceDefs.h"
#include "Packing.h"
#include <memory>

namespace Gfx
{
    //Class comprised of many different images all placed into one maximum sized texture
    // contains functions allowing you to pack data in to it
    class TextureAtlas
    {
    public:
        TextureAtlas(uint32_t width, uint32_t height, std::string const& label);

        bool AddToAtlas(AnimationSet& animationSet, std::vector<TextureResource> const& textures);
        void CompletePacking();

        TextureResource const& GetTexture(){ return _atlas;}

        bool IsPackingCompleted(){ return _packer == nullptr;}
        uint8_t Id(){return _id;}
    private:
        TextureResource _atlas;
        //created on atlas construction, destroyed when CompletePacking is called
        std::unique_ptr<SkylinePacker> _packer;
        std::string _label;
        uint8_t _id;
    };
}