/*
 * Link List Header for UWB Tag
 * Manages multiple anchor connections
 */

#ifndef LINK_H
#define LINK_H

#include <Arduino.h>

// Linked list node structure for anchor data
struct MyLink
{
    uint16_t anchor_addr;    // Anchor short address (2 bytes)
    float range[3];          // Range history for moving average (meters)
    float dbm;               // Signal strength (dBm)
    struct MyLink *next;     // Pointer to next node
};

// Core linked list functions
struct MyLink *init_link();
void add_link(struct MyLink *p, uint16_t addr);
struct MyLink *find_link(struct MyLink *p, uint16_t addr);
void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm);
void print_link(struct MyLink *p);
void delete_link(struct MyLink *p, uint16_t addr);
void make_link_json(struct MyLink *p, String *s, int co2Value);
void free_all_links(struct MyLink *p);

// Utility functions
int count_links(struct MyLink *p);

#endif
