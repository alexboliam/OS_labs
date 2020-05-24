using System;
using System.Collections.Generic;
using System.Text;

namespace MyQueue
{
    /// <summary>
    /// Node for generic linked list
    /// </summary>
    /// <typeparam name="T">Type of data stored in list</typeparam>
    public class Node<T>
    {
        public Node(T Value)
        {
            this.Value = Value;
        }
        public T Value { get; set; }
        public Node<T> Next { get; set; }
    }
}
