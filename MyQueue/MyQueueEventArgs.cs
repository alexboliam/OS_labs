using System;
using System.Collections.Generic;
using System.Text;

namespace MyQueue
{
    public class MyQueueEventArgs<T> : EventArgs
    {
        public T Value { get; set; }

        public MyQueueEventArgs(T Value)
        {
            this.Value = Value;
        }
    }
}
