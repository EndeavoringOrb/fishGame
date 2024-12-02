#ifndef FISH_BOOK_HPP
#define FISH_BOOK_HPP

#include "fish.hpp"

struct FishEntry
{
    std::string name;
    int coinValue;

    int numCaught = 0;
    bool unlocked = false;

    FishEntry(std::string _name, int _coinValue)
        : name(_name),
          coinValue(_coinValue) {}
};

struct FishBook
{
    std::vector<FishEntry> entries;

    int update(std::vector<Fish> &newFish)
    {
        int newCoins = 0;

        for (Fish &fish : newFish)
        {
            for (FishEntry &entry : entries)
            {
                if (fish.name == entry.name)
                {
                    entry.numCaught++;
                    entry.unlocked = true;
                    newCoins += entry.coinValue;
                    break;
                }
            }
        }

        return newCoins;
    }

    int getValue(const Fish &fish)
    {
        for (const FishEntry &entry : entries)
        {
            if (fish.name == entry.name)
            {
                return entry.coinValue;
            }
        }
        return 0;
    }
};

#endif