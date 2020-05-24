using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace MyQueue
{
    public class MyQueueEnumerator<T> : IEnumerator<T>
    {
        private MyQueue<T> Queue;
        private T[] array;
        private int current = -1;
        private int Count = 0;

        public T Current
        { get
            {
                return array[current];
            }
        }
        object IEnumerator.Current { get { return Current; } }

        public MyQueueEnumerator(MyQueue<T> Queue)
        {
            this.Count = Queue.Count;
            this.Queue = Queue;
            array = new T[this.Count];
            Queue.CopyTo(array, 0);
        }
        public void Dispose() { }
        public bool MoveNext()
        {
            if (current < Count - 1)
            {
                current++;
                return true;
            }
            return false;
        }
        public void Reset() { throw new ArgumentException("This method is not supperted for queue type"); }
    }
}
