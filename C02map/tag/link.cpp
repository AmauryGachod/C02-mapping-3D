/*
 * Link List Implementation for UWB Tag
 * Thread-safe linked list operations
 */

#include "link.h"

//#define SERIAL_DEBUG

// ============================================================
// INITIALIZE LINKED LIST
// ============================================================
struct MyLink *init_link()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_link");
#endif
    struct MyLink *p = (struct MyLink *)malloc(sizeof(struct MyLink));
    
    if (p == NULL)
    {
        Serial.println("ERROR: init_link malloc failed");
        return NULL;
    }
    
    // Initialize head node
    p->next = NULL;
    p->anchor_addr = 0;
    p->range[0] = 0.0f;
    p->range[1] = 0.0f;
    p->range[2] = 0.0f;
    p->dbm = 0.0f;

    return p;
}

// ============================================================
// ADD NEW ANCHOR TO LIST
// ============================================================
void add_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_link");
#endif
    
    if (p == NULL) return;
    
    // Count existing anchors and find end
    int count = 0;
    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        count++;
        temp = temp->next;
    }
    
    // Limit to 3 anchors for trilateration
    if (count >= 3)
    {
        Serial.println("WARNING: Maximum 3 anchors reached");
        return;
    }

    Serial.println("add_link: Adding new anchor to list");
    struct MyLink *a = (struct MyLink *)malloc(sizeof(struct MyLink));
    
    if (a == NULL)
    {
        Serial.println("ERROR: add_link malloc failed");
        return;
    }
    
    // Initialize new node
    a->anchor_addr = addr;
    a->range[0] = 0.0f;
    a->range[1] = 0.0f;
    a->range[2] = 0.0f;
    a->dbm = 0.0f;
    a->next = NULL;

    temp->next = a;
}

// ============================================================
// FIND ANCHOR BY ADDRESS
// ============================================================
struct MyLink *find_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_link");
#endif
    if (addr == 0)
    {
        return NULL;
    }

    if (p == NULL || p->next == NULL)
    {
        return NULL;
    }

    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        temp = temp->next;
        if (temp->anchor_addr == addr)
        {
            return temp;
        }
    }

    return NULL;
}

// ============================================================
// UPDATE ANCHOR RANGE WITH MOVING AVERAGE
// ============================================================
void fresh_link(struct MyLink *p, uint16_t addr, float range, float dbm)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_link");
#endif
    struct MyLink *temp = find_link(p, addr);
    if (temp != NULL)
    {
        // Calculate moving average BEFORE shifting
        float averaged = (range + temp->range[0] + temp->range[1]) / 3.0f;
        
        // Shift history
        temp->range[2] = temp->range[1];
        temp->range[1] = temp->range[0];
        temp->range[0] = averaged;
        temp->dbm = dbm;
    }
}

// ============================================================
// PRINT ALL ANCHORS (DEBUG)
// ============================================================
void print_link(struct MyLink *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_link");
#endif
    if (p == NULL) return;
    
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        Serial.print("Anchor: 0x");
        Serial.print(temp->next->anchor_addr, HEX);
        Serial.print("\t Range: ");
        Serial.print(temp->next->range[0], 2);
        Serial.print(" m\t dBm: ");
        Serial.println(temp->next->dbm, 1);
        temp = temp->next;
    }
}

// ============================================================
// DELETE ANCHOR FROM LIST
// ============================================================
void delete_link(struct MyLink *p, uint16_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_link");
#endif
    if (addr == 0 || p == NULL)
        return;

    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        if (temp->next->anchor_addr == addr)
        {
            struct MyLink *del = temp->next;
            temp->next = del->next;
            free(del);
            Serial.print("Deleted anchor: 0x");
            Serial.println(addr, HEX);
            return;
        }
        temp = temp->next;
    }
}

// ============================================================
// GENERATE JSON STRING (ZERO HEAP FRAGMENTATION)
// ============================================================
void make_link_json(struct MyLink *p, String *s, int co2Value)
{
    // Use stack buffer to prevent heap fragmentation
    char buffer[512];
    int offset = 0;
    
    offset += sprintf(buffer + offset, "[");
    
    struct MyLink *temp = p;
    bool first = true;

    while (temp->next != NULL && offset < 470)
    {
        temp = temp->next;
        if (!first) 
            offset += sprintf(buffer + offset, ",");
        first = false;

        // Format: {"T":timestamp,"A":"address","R":range,"Rx":rssi,"C":co2}
        offset += sprintf(buffer + offset,
                "{\"T\":%lu,\"A\":\"%04X\",\"R\":%.2f,\"Rx\":%.1f,\"C\":%d}",
                millis(), 
                temp->anchor_addr & 0xFFFF, 
                temp->range[0], 
                temp->dbm, 
                co2Value);
    }

    sprintf(buffer + offset, "]");
    
    // Pre-reserve String memory to avoid reallocation
    s->reserve(offset + 2);
    *s = String(buffer);
    
#ifdef SERIAL_DEBUG
    Serial.print("JSON length: ");
    Serial.println(s->length());
#endif
}

// ============================================================
// FREE ENTIRE LIST (CLEANUP)
// ============================================================
void free_all_links(struct MyLink *p)
{
    if (p == NULL) return;
    
    int count = 0;
    struct MyLink *current = p->next;
    while (current != NULL)
    {
        struct MyLink *next = current->next;
        free(current);
        current = next;
        count++;
    }
    p->next = NULL;
    
    Serial.print("Freed ");
    Serial.print(count);
    Serial.println(" links");
}

// ============================================================
// COUNT ANCHORS IN LIST
// ============================================================
int count_links(struct MyLink *p)
{
    if (p == NULL) return 0;
    
    int count = 0;
    struct MyLink *temp = p->next;
    while (temp != NULL)
    {
        count++;
        temp = temp->next;
    }
    return count;
}
