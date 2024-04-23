#include "queue.c"

int main() {
    queue* new_queue = create_queue();
    char message[] = {"asdas"};
    char message_2[] = {"dsdf"};
    add_new_message(new_queue, message, 1, strlen(message));
    add_new_message(new_queue, message_2, 1, strlen(message_2));
    delete_message(new_queue);
    delete_message(new_queue);
    return 0;
}