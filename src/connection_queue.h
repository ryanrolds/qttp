
class ConnectionQueue {
 private:
  

 public:
  ConnectionQueue();
  int Push(Connection);
  connection* Pop();
  ~ConnectionQueue();
}
