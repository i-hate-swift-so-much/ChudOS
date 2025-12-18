#pragma once

#include "stdint.h"
#include "stddef.h"

struct LinkedListNode{
    int Value;
    int Index;
    LinkedListNode* next;
};

class LinkedList{
    private:
        int size_of;
        LinkedListNode* main_node = new LinkedListNode;
        int& SearchFor(int index);

    public:
        LinkedList();
        int& operator[](int index) {
            if (index < 0 || index >= size_of) {
                // Handle error appropriately, e.g., throw an exception or return a default value
                // For simplicity, we'll return a reference to a static dummy value here.
                static int dummy = -1; 
                return dummy; 
            }
            return SearchFor(index);
        }
        
        LinkedList& operator=(const int* arraySet){
            int size = sizeof(arraySet);

            LinkedListNode* last_node = new LinkedListNode;
                    
            for(int i = 0; i < size; i++){
                LinkedListNode* newNode = new LinkedListNode;
                newNode->Index = size_of;
                newNode->Value = arraySet[i];
                if(i > 0){
                    last_node->next = newNode;
                    last_node = new LinkedListNode;
                    last_node = newNode;
                    delete newNode;
                }else{
                    main_node = newNode;
                }
            }
            return *this;
    }
};