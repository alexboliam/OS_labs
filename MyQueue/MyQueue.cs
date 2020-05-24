using System;
using System.Collections.Generic;
using System.Text;
using System.Collections;

namespace MyQueue
{
    /// <summary>
    /// Generic collection with FIFO principle
    /// </summary>
    /// <typeparam name="T">Type of data stored in queue</typeparam>
    public class MyQueue<T> : IEnumerable<T>, ICollection<T>
    {
        #region Private fields

        private Node<T> head;
        private Node<T> rear;

        #endregion

        #region Properties
        public int Count { get; private set; } = 0;
        public bool IsReadOnly { get; } = false;
        #endregion

        #region Events
        public delegate void EventHandler(object sender, MyQueueEventArgs<T> e);
        public event EventHandler OnClear;
        public event EventHandler OnAdd;
        public event EventHandler OnRemove;
        #endregion

        #region Ctors
        public MyQueue() { }
        public MyQueue(IEnumerable<T> collection)
        {
            foreach(var item in collection)
            {
                Enqueue(item);
            }
        }

        #endregion

        #region Public methods
        public void Enqueue(T Value)
        {
            Node<T> node = new Node<T>(Value);
            var tmpNode = this.rear;
            this.rear = node;

            if (Count == 0) { this.head = this.rear; }
            else { tmpNode.Next = this.rear; }

            Count++;

            OnAdd?.Invoke(this, new MyQueueEventArgs<T>(Value));
        }
        public T Dequeue()
        {
            if (Count == 0) throw new Exception("Queue is empty");

            T Value = this.head.Value;
            this.head = this.head.Next;
            Count--;

            OnRemove?.Invoke(this, new MyQueueEventArgs<T>(Value));

            return Value;
        }
        public T Peek()
        {
            if (Count == 0) throw new ArgumentException("Queue is empty");

            return this.head.Value;
        }
        public void Clear()
        {
            this.head = null;
            this.rear = null;
            Count = 0;

            OnClear?.Invoke(this, null);
        }
        public bool Contains(T value)
        {
            var current = this.head;
            while (current != null)
            {
                if (current.Value.Equals(value))
                {
                    return true;
                }
                else
                {
                    current = current.Next;
                }
            }
            return false;
        }

        public void CopyTo(T[] array, int index)
        {
            if (index > array.Length - 1 || index < 0 || Count == 0)
                throw new ArgumentException("Index is out of bounds for the array or queue is empty");
            if (array.Length == 0)
                throw new ArgumentNullException("The array cannot be null");
            if (Count > array.Length - index)
                throw new ArgumentException("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection.");

            var current = this.head;

            for (int i = index, j = 0; j < Count; i++, j++)
            {
                array[i] = current.Value;
                current = current.Next;
            }
        }
        public void Add(T item) => Enqueue(item);
        public bool Remove(T item)
        {
            if (this.head.Value.Equals(item))
            {
                this.head = this.head.Next;
                Count--;
                OnRemove?.Invoke(this, new MyQueueEventArgs<T>(item));
                return true;
            }
            else
            {
                throw new ArgumentException("You can not remove specified value if it is not head of the queue");
            }
        }

        public IEnumerator<T> GetEnumerator() => new MyQueueEnumerator<T>(this);

        IEnumerator IEnumerable.GetEnumerator()
        {
            return new MyQueueEnumerator<T>(this);
        }
        #endregion

    }
}
