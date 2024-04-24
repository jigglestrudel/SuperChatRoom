#include <unistd.h>
#include <stdlib.h>

typedef struct node {
    struct node* next;  //closer to end
    struct node* previous; //closer to start
    int user_number;
    char* message;
    int message_length;
    int message_type;
} node;

typedef struct queue {
    node* start;  //first to delete
    node* end;   //last to delete
} queue;

node* create_new_message_node(char* message, int user_number, int message_length) {
    node* new_node = (node*)malloc(sizeof(node));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->message = message;
    new_node->user_number = user_number;
    new_node->message_length = message_length;
    new_node->next = NULL;
    new_node->previous = NULL;
    return new_node;
}

queue* create_queue() {
    //empty queue initialization
    queue* new_queue = (queue*)malloc(sizeof(queue));
    if (new_queue == NULL) {
        return NULL;
    }
    new_queue->end = NULL;
    new_queue->start = NULL;
    return new_queue;
}

void add_new_message(queue* queue, char* new_message, int user_number, int message_length) {
    node* new_message_node = create_new_message_node(new_message, user_number, message_length);
    if (new_message_node != NULL) {
        if (queue->end == NULL) {  //empty queue
        queue->end = new_message_node;
        queue->start = new_message_node;
        }
        else {
            node* previous_node = queue->end;
            previous_node->next = new_message_node;
            new_message_node->previous = previous_node;
            queue->end = new_message_node;
        }
    }
}

void delete_message(queue* queue) {
    if (queue->start != NULL) {
        node* to_delete = queue->start;
        if (queue->start->next != NULL) {  //if there is no next node
            node* next_node = queue->start->next;
            queue->start = next_node;
            next_node->previous = NULL;
        }
        else {
            queue->start = NULL;
            queue->end = NULL;
        }
        free(to_delete->message);
        free(to_delete);
    }
}