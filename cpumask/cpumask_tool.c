/*
 * CPU Mask Calculator Tool
 * Similar to taskset for SPDK cpumask calculation
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_CORES 1024
#define MAX_INPUT_LEN 4096

typedef struct {
    uint64_t mask[MAX_CORES / 64];  // Support up to 1024 cores
    int max_core;
} cpumask_t;

// Initialize cpumask
void cpumask_init(cpumask_t *mask) {
    memset(mask, 0, sizeof(*mask));
    mask->max_core = -1;
}

// Set a specific core in the mask
void cpumask_set_core(cpumask_t *mask, int core) {
    if (core < 0 || core >= MAX_CORES) {
        fprintf(stderr, "Error: Core %d is out of range (0-%d)\n", core, MAX_CORES - 1);
        return;
    }
    
    int word_idx = core / 64;
    int bit_idx = core % 64;
    
    mask->mask[word_idx] |= (1ULL << bit_idx);
    if (core > mask->max_core) {
        mask->max_core = core;
    }
}

// Check if a core is set in the mask
int cpumask_is_set(const cpumask_t *mask, int core) {
    if (core < 0 || core >= MAX_CORES) {
        return 0;
    }
    
    int word_idx = core / 64;
    int bit_idx = core % 64;
    
    return (mask->mask[word_idx] & (1ULL << bit_idx)) != 0;
}

// Parse core list like "0,2,4-7,10-12"
int parse_core_list(const char *core_list, cpumask_t *mask) {
    char *input = strdup(core_list);
    char *token = strtok(input, ",");
    
    while (token != NULL) {
        // Remove whitespace
        while (isspace(*token)) token++;
        
        char *dash = strchr(token, '-');
        if (dash != NULL) {
            // Range like "4-7"
            *dash = '\0';
            int start = atoi(token);
            int end = atoi(dash + 1);
            
            if (start > end) {
                fprintf(stderr, "Error: Invalid range %d-%d\n", start, end);
                free(input);
                return -1;
            }
            
            for (int i = start; i <= end; i++) {
                cpumask_set_core(mask, i);
            }
        } else {
            // Single core
            int core = atoi(token);
            cpumask_set_core(mask, core);
        }
        
        token = strtok(NULL, ",");
    }
    
    free(input);
    return 0;
}

// Parse hex cpumask like "0xff" or "ff"
int parse_hex_mask(const char *hex_str, cpumask_t *mask) {
    const char *str = hex_str;
    
    // Skip "0x" prefix if present
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        str += 2;
    }
    
    int len = strlen(str);
    if (len == 0) {
        fprintf(stderr, "Error: Empty hex string\n");
        return -1;
    }
    
    // Parse from right to left (least significant bits first)
    for (int i = len - 1, bit_pos = 0; i >= 0 && bit_pos < MAX_CORES; i--, bit_pos += 4) {
        char c = str[i];
        int nibble;
        
        if (c >= '0' && c <= '9') {
            nibble = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            nibble = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            nibble = c - 'A' + 10;
        } else {
            fprintf(stderr, "Error: Invalid hex character '%c'\n", c);
            return -1;
        }
        
        // Set bits for this nibble
        for (int j = 0; j < 4 && bit_pos + j < MAX_CORES; j++) {
            if (nibble & (1 << j)) {
                cpumask_set_core(mask, bit_pos + j);
            }
        }
    }
    
    return 0;
}

// Print cpumask as hex string
void print_hex_mask(const cpumask_t *mask) {
    if (mask->max_core < 0) {
        printf("0x0\n");
        return;
    }
    
    // Calculate how many hex digits we need
    int max_word = mask->max_core / 64;
    
    printf("0x");
    
    // Print from most significant to least significant
    int printed = 0;
    for (int word = max_word; word >= 0; word--) {
        if (word == max_word) {
            // For the highest word, only print necessary digits
            uint64_t val = mask->mask[word];
            if (val == 0 && word > 0) continue;
            
            printf("%llx", (unsigned long long)val);
            printed = 1;
        } else if (printed) {
            // For lower words, always print 16 hex digits (64 bits)
            printf("%016llx", (unsigned long long)mask->mask[word]);
        }
    }
    
    if (!printed) {
        printf("0");
    }
    
    printf("\n");
}

// Print core list
void print_core_list(const cpumask_t *mask) {
    int first = 1;
    int range_start = -1;
    int range_end = -1;
    
    for (int i = 0; i <= mask->max_core + 1; i++) {
        int is_set = (i <= mask->max_core) ? cpumask_is_set(mask, i) : 0;
        
        if (is_set) {
            if (range_start == -1) {
                range_start = i;
                range_end = i;
            } else {
                range_end = i;
            }
        } else {
            if (range_start != -1) {
                if (!first) printf(",");
                
                if (range_start == range_end) {
                    printf("%d", range_start);
                } else if (range_end == range_start + 1) {
                    printf("%d,%d", range_start, range_end);
                } else {
                    printf("%d-%d", range_start, range_end);
                }
                
                first = 0;
                range_start = -1;
            }
        }
    }
    
    if (first) {
        printf("(no cores selected)");
    }
    
    printf("\n");
}

// Count number of cores in mask
int count_cores(const cpumask_t *mask) {
    int count = 0;
    for (int i = 0; i <= mask->max_core; i++) {
        if (cpumask_is_set(mask, i)) {
            count++;
        }
    }
    return count;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("CPU Mask Calculator Tool - Convert between core lists and hex masks\n\n");
    printf("Options:\n");
    printf("  -c, --cores <list>     Specify cores as comma-separated list or ranges\n");
    printf("                         Examples: '0,2,4-7', '0-3,8,10-15'\n");
    printf("  -m, --mask <hex>       Specify cpumask as hexadecimal value\n");
    printf("                         Examples: '0xff', 'ff', '0x123abc'\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --verbose          Show detailed information\n\n");
    printf("Examples:\n");
    printf("  %s -c '0,2,4-7'        # Convert core list to mask\n", program_name);
    printf("  %s -m 0xff             # Convert mask to core list\n", program_name);
    printf("  %s -c '0-3' -v         # Verbose output\n", program_name);
    printf("\nOutput:\n");
    printf("  - Hex mask (for SPDK --cpumask parameter)\n");
    printf("  - Core list (human readable)\n");
    printf("  - Core count\n");
}

int main(int argc, char *argv[]) {
    cpumask_t mask;
    char *core_list = NULL;
    char *hex_mask = NULL;
    int verbose = 0;
    int show_help = 0;
    
    cpumask_init(&mask);
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cores") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -c requires an argument\n");
                return 1;
            }
            core_list = argv[++i];
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mask") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -m requires an argument\n");
                return 1;
            }
            hex_mask = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (show_help || (core_list == NULL && hex_mask == NULL)) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (core_list != NULL && hex_mask != NULL) {
        fprintf(stderr, "Error: Cannot specify both -c and -m options\n");
        return 1;
    }
    
    // Parse input
    if (core_list != NULL) {
        if (parse_core_list(core_list, &mask) != 0) {
            return 1;
        }
        if (verbose) {
            printf("Parsed core list: %s\n", core_list);
        }
    } else if (hex_mask != NULL) {
        if (parse_hex_mask(hex_mask, &mask) != 0) {
            return 1;
        }
        if (verbose) {
            printf("Parsed hex mask: %s\n", hex_mask);
        }
    }
    
    // Output results
    if (verbose) {
        printf("\nResults:\n");
        printf("--------\n");
    }
    
    printf("Hex mask:   ");
    print_hex_mask(&mask);
    
    printf("Core list:  ");
    print_core_list(&mask);
    
    printf("Core count: %d\n", count_cores(&mask));
    
    if (verbose) {
        printf("\nUsage with SPDK:\n");
        printf("  --cpumask ");
        print_hex_mask(&mask);
        
        printf("\nUsage with taskset:\n");
        printf("  taskset -c ");
        print_core_list(&mask);
    }
    
    return 0;
} 