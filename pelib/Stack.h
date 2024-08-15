#pragma once

#ifndef STACKHANDLE_H
#define STACKHANDLE_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define USE_MEMORY_LEAK_DETECTED
#include "Memory.h"

#define MAX_PATH_LENGTH 128

typedef struct StackNode {
    char* path;
    struct StackNode* next;
} StackNode;

typedef struct Stack {
    StackNode* top;
} Stack;

void stackPush(Stack* stack, const char* path) {
    StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));
    if (newNode == NULL)
        return;
    newNode->path = (char*)malloc(strlen(path) + 1);
    if (newNode->path == NULL) {
        free(newNode);
        return;
    }
    strcpy(newNode->path, path);
    newNode->next = stack->top;
    stack->top = newNode;
}

char* stackPop(Stack* stack) {
    if (stack->top == NULL) {
        return NULL;
    }
    StackNode* topNode = stack->top;
    char* path = (char*)malloc(strlen(topNode->path) + 1);
    if (path == NULL)
        return NULL;
    strcpy(path, topNode->path);
    stack->top = topNode->next;
    free(topNode->path);
    free(topNode);
    return path;
}

void stackClear(Stack* stack) {
    while (stack->top != NULL) {
        StackNode* topNode = stack->top;
        stack->top = topNode->next;
        free(topNode->path);
        free(topNode);
    }
}

#endif