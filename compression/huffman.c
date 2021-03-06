#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"

/**
 * Binary tree direction enumerator to use in creation of symbols bit path dictionary.
 */
typedef enum binary_tree_direction { ROOT, LEFT, RIGHT } BINARY_TREE_DIRECTION;

/**
 * Allocates memory to a symbol frequency struct.
 *
 * @param symbol character.
 * @param frequency character frequency.
 * @return pointer to {@link symbol_frequency_t}.
 */
symbol_frequency_t* create_symbol_frequency(unsigned char symbol, int frequency);

/**
 * Traverses binary tree in pre order returning the bit paths of leaves.
 *
 * @param symbol_frequency_tree pointer to root node of a symbol frequency tree.
 * @param direction binary tree direction.
 * @param traversal_bits string used to make the bit paths during traversal recursion. Must initially receive NULL.
 * @param on_bit_path pointer to a function that receive the symbol and its respective bit path string.
 */
void find_bit_paths(
        binary_tree_t *symbol_frequency_tree,
        BINARY_TREE_DIRECTION direction,
        char *traversal_bits,
        void (*on_bit_path)(unsigned char symbol, char *bit_path)
);


hashtable_t* make_symbol_frequency_map(char *file_path)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        return NULL;
    }
    hashtable_t *hashtable = hashtable_create(ASCII_TABLE_SIZE, sizeof(int));
    unsigned char c;

    while (fscanf(file, "%c", &c) != EOF) {
        void *value = hashtable_get(hashtable, c);
        if (value == NULL) {
            int *frequency = (int*) malloc(sizeof(int));
            *frequency = 1;
            hashtable_put(hashtable, c, frequency);
        } else {
            *((int*) value) += 1;
        }
    }
    return hashtable;
}

queue_t* make_leaves_priority_queue(hashtable_t *symbol_frequency_map)
{
    if (symbol_frequency_map == NULL) {
        return NULL;
    }
    queue_t *priority_queue = queue_create();

    void on_pair(int key, void *value)
    {
        unsigned char symbol = key;
        int frequency = *((int*) value);
        symbol_frequency_t *symbol_frequency = create_symbol_frequency(symbol, frequency);
        binary_tree_t *leaf = binary_tree_create(symbol_frequency, NULL, NULL);

        priority_queue_enqueue(priority_queue, leaf, frequency, ASC);
    }

    hashtable_iterate(symbol_frequency_map, on_pair);
    return priority_queue;
}

binary_tree_t* make_symbol_frequency_tree(queue_t *leaves_priority_queue)
{
    if (leaves_priority_queue == NULL || queue_is_empty(leaves_priority_queue)) {
        return NULL;
    }
    while (queue_size(leaves_priority_queue) > 1) {
        binary_tree_t *leaf1 = priority_queue_dequeue(leaves_priority_queue)->data;
        binary_tree_t *leaf2 = priority_queue_dequeue(leaves_priority_queue)->data;

        symbol_frequency_t *symbol_frequency1 = (symbol_frequency_t*) leaf1->data;
        symbol_frequency_t *symbol_frequency2 = (symbol_frequency_t*) leaf2->data;

        symbol_frequency_t *parent_symbol_frequency = create_symbol_frequency(
                INTERNAL_NODE_SYMBOL,
                symbol_frequency1->frequency + symbol_frequency2->frequency
        );
        binary_tree_t *parent_tree = binary_tree_create(parent_symbol_frequency, leaf1, leaf2);

        priority_queue_enqueue(leaves_priority_queue, parent_tree, parent_symbol_frequency->frequency, ASC);
    }
    return priority_queue_dequeue(leaves_priority_queue)->data;
}

hashtable_t* make_symbol_bits_map(binary_tree_t *symbol_frequency_tree)
{
    if (symbol_frequency_tree == NULL) {
        return NULL;
    }
    hashtable_t *hashtable = hashtable_create(ASCII_TABLE_SIZE, sizeof(char*));

    void on_bit_path(unsigned char symbol, char *bit_path)
    {
        hashtable_put(hashtable, symbol, bit_path);
    }

    find_bit_paths(symbol_frequency_tree, ROOT, NULL, on_bit_path);
    return hashtable;
}

binary_tree_t* tree_from_pre_order_file(FILE *file, int file_offset, short tree_size)
{
    if (ftell(file) - file_offset >= tree_size) {
        return NULL;
    }
    unsigned char symbol = fgetc(file);
    unsigned char *symbol_copy = (unsigned char*) malloc(sizeof(char));
    *symbol_copy = symbol;

    binary_tree_t *node = NULL;

    if (symbol == INTERNAL_NODE_SYMBOL) {
        node = binary_tree_create(symbol_copy, NULL, NULL);
        node->left = tree_from_pre_order_file(file, file_offset, tree_size);
        node->right = tree_from_pre_order_file(file, file_offset, tree_size);
    } else {
        if (symbol == SCAPE_SYMBOL) {
            symbol = fgetc(file);
            *symbol_copy = symbol;
        }
        node = binary_tree_create(symbol_copy, NULL, NULL);
    }
    return node;
}


symbol_frequency_t* create_symbol_frequency(unsigned char symbol, int frequency)
{
    symbol_frequency_t *symbol_frequency = (symbol_frequency_t*) malloc(sizeof(symbol_frequency_t));
    symbol_frequency->symbol = symbol;
    symbol_frequency->frequency = frequency;

    return symbol_frequency;
}

void find_bit_paths(
        binary_tree_t *symbol_frequency_tree,
        BINARY_TREE_DIRECTION direction,
        char *traversal_bits,
        void (*on_bit_path)(unsigned char symbol, char *bit_path)
)
{
    if (direction == ROOT) {
        traversal_bits = (char*) malloc(sizeof(char));
        traversal_bits[0] = '\0';
        find_bit_paths(symbol_frequency_tree->left, LEFT, traversal_bits, on_bit_path);
        find_bit_paths(symbol_frequency_tree->right, RIGHT, traversal_bits, on_bit_path);
    } else {
        int new_length = strlen(traversal_bits) + 2;
        traversal_bits = (char*) realloc(traversal_bits, sizeof(char) * new_length);
        traversal_bits[new_length - 2] = direction == LEFT ? '0' : '1';
        traversal_bits[new_length - 1] = '\0';

        if (binary_tree_is_leaf(symbol_frequency_tree)) {
            unsigned char symbol = ((symbol_frequency_t*) symbol_frequency_tree->data)->symbol;
            char *copy = (char*) malloc(sizeof(char) * (strlen(traversal_bits) + 1));

            strcpy(copy, traversal_bits);
            on_bit_path(symbol, copy);
        } else {
            find_bit_paths(symbol_frequency_tree->left, LEFT, traversal_bits, on_bit_path);
            find_bit_paths(symbol_frequency_tree->right, RIGHT, traversal_bits, on_bit_path);
        }
        // Removing last character
        new_length = strlen(traversal_bits);
        traversal_bits = (char*) realloc(traversal_bits, sizeof(char) * new_length);
        traversal_bits[new_length - 1] = '\0';
    }
}
