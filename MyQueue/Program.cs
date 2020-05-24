using System;
using System.Threading;

namespace MyQueue
{
    public class Program
    {

        static void Main(string[] args)
        {
            MyQueue<int> queue = new MyQueue<int>();
            for(int i = 0; i< 30; i++)
            {
                queue.Enqueue(new Random().Next(20, 500));
            }

            int number = 0;

            foreach(var time in queue)
            {
                number++;
                Thread.Sleep(time);
                Console.WriteLine("Заявка №" + number + " заняла " + time + "мс и покинула очередь.");
            }

        }
        
        public static void OnQueueAdd<T>(object sender, MyQueueEventArgs<T> e)
        {
            Console.WriteLine("Element added to queue: " + e.Value.ToString());
        }
        public static void OnQueueRemove<T>(object sender, MyQueueEventArgs<T> e)
        {
            Console.WriteLine("Element removed from queue: " + e.Value.ToString());
        }
        public static void OnQueueClear<T>(object sender, MyQueueEventArgs<T> e)
        {
            Console.WriteLine("Queue was cleared");
        }
    }
    
}
