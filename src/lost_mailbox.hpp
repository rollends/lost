#ifndef MAILBOX_HPP
#define MAILBOX_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>


template< typename MessageType >
struct Mail
{
    Mail* next;
    MessageType payload;
};

/*!
 * MailBox structure.
 */
template< typename MessageType,
          int InitialBufferSize >
struct MailBox
{
    using Mail = struct Mail<MessageType>;

    void send(MessageType const & data)
    {
        Mail* envelope = allocateNode();
        envelope->payload = data;

        {
            std::unique_lock<std::mutex> lock(mutQueue);
            if( apTail )
            {
                apTail->next = envelope;
                apTail = envelope;
            }
            else
            {
                apTail = envelope;
                apHead = envelope;
            }
        }
    }

    bool receive(MessageType& data)
    {
        Mail* node;

        {
            std::unique_lock<std::mutex> lock(mutQueue);

      if( !apHead ) { return false; }

            node = apHead;
            apHead = node->next;
            if( !apHead )
            {
                apTail = apHead;
            }
        }
        data = node->payload;
        freeNode(node);
        return true;
    }

    MailBox()
      : apFreeList(buffer + 1),
        apHead(nullptr),
        apTail(nullptr)
    {
        int i;
        for(i = 0; i < InitialBufferSize - 1; ++i)
        {
            buffer[i].next = buffer + i + 1;
        }
        buffer[i].next = nullptr;
    }

private:
    Mail* allocateNode()
    {
        // First get a free block.
        Mail* freeList = nullptr;
        Mail* node = nullptr;

        // Loop while the free list is NON-empty AND we fail
        // to properly pop the node from the list!
        do
        {
            freeList = apFreeList;
            if( !freeList )
            {
                node = new Mail;
                break;
            }
            node = freeList;
        } while( !apFreeList.compare_exchange_weak(freeList, node->next) );

        node->next = nullptr;
        return node;
    }

    void freeNode( Mail* node )
    {
        Mail* freeList;
        do
        {
            freeList = apFreeList;
            node->next = freeList;
        } while( !apFreeList.compare_exchange_weak(freeList, node) );
    }

    std::atomic<Mail*> apFreeList;
    Mail* apHead;
    Mail* apTail;
    std::mutex mutQueue;
    Mail buffer[InitialBufferSize];
};

#endif
