#include "gamemodel.h"
#include <QQueue>
#include <QDebug>
#include <QMessageBox>
#include <ctime>
#include <cstdlib>
using std::srand;
using std::rand;
using std::time;

int TIME_LIMIT = 30; //每步棋时间限制
int LINE_NUM = 9; //棋盘横线或竖线的数量

struct point{
    int x;
    int y;
};

GameModel::GameModel()
{
    //初始化
    for(int i = 0;i <= LINE_NUM+1;i++){
        for(int j = 0;j <= LINE_NUM+1;j++){
            board[i][j] = 3;
            qi[i][j] = 0;
        }
    }
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            board[i][j] = 0;
        }
    }
    state = Playing;
    type = PVP;
    player = Black;
}

GameModel::~GameModel()
{

}

void GameModel::updateQi(int lx, int ly, int player)
{
    //每下一步棋后，更新各棋子的“气”
    //lx,ly分别为最近一次落子的坐标,player为1表示为黑棋下的，player为2表示为白棋下的

    int sides[4][2] = {{-1,0},{1,0},{0,1},{0,-1}}; //表示上下左右四个方向

    //更新邻近异色棋子的气
        //先更新紧靠落子的异色棋
    for(int a = 0;a < 4;a++){
        int i = sides[a][0];
        int j = sides[a][1];
        if(board[lx+i][ly+j] == 3 - player){
            qi[lx+i][ly+j]--;
        }
    }
        //再用BFS更新紧靠落子的异色棋的邻近棋
    int isvisited[20][20] = {{0}}; //表示某个异色棋是否被访问过
    for(int a = 0;a < 4;a++){
        int i = sides[a][0];
        int j = sides[a][1];
        if(board[lx+i][ly+j] == 3 - player && isvisited[lx+i][ly+j] == 0){
            int curqi = qi[lx+i][ly+j]; //存储当前棋子的气
            QQueue<point> q;
            point p{lx+i,ly+j};
            q.push_back(p);
            isvisited[lx+i][ly+j] = 1;
            while(q.empty() == 0){
                p = q.front();
                q.pop_front();
                for(int b = 0;b < 4;b++){
                    int m = sides[b][0];
                    int n = sides[b][1];
                    if(board[p.x+m][p.y+n] == 3 - player && isvisited[p.x+m][p.y+n] == 0){
                        q.push_back(point{p.x+m,p.y+n});
                        isvisited[p.x+m][p.y+n] = 1;
                        qi[p.x+m][p.y+n] = curqi;
                    }
                }
            }
        }
    }

    //更新邻近同色棋子的气
    for(int i = 0;i < LINE_NUM+2;i++){
        for(int j = 0;j < LINE_NUM+2;j++){
            isvisited[i][j] = 0; //重置isvisited数组,且此时该数组表示某个空位是否被访问过
        }
    }
    int is_the_same_player[20][20] = {{0}}; //记录在落子旁边且与落子同色的棋子的位置
    int totalqi = 0;
    QQueue<point> q;
    point p{lx,ly};
    q.push_back(p);
    is_the_same_player[lx][ly] = 1;
    while(q.empty() == 0){
        p = q.front();
        q.pop_front();
        for(int b = 0;b < 4;b++){
            int m = sides[b][0];
            int n = sides[b][1];
            if(board[p.x+m][p.y+n] == 0 && isvisited[p.x+m][p.y+n] == 0){
                totalqi++;
                isvisited[p.x+m][p.y+n] = 1;
            }
        }
        for(int b = 0;b < 4;b++){
            int m = sides[b][0];
            int n = sides[b][1];
            if(board[p.x+m][p.y+n] == player && is_the_same_player[p.x+m][p.y+n] == 0){
                q.push_back(point{p.x+m,p.y+n});
                is_the_same_player[p.x+m][p.y+n] = 1;
            }
        }
    }

    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(is_the_same_player[i][j] == 1){
                qi[i][j] = totalqi;
            }
        }
    }

}


void GameModel::checkWin(int player)
{
    //player为刚刚落子的一方

    //判断是否分出胜负
    bool isWin = false;
    //只要有棋子气变为0，不管是黑棋还是白棋，都是对手赢
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(board[i][j] != 0 && board[i][j] != 3 && qi[i][j] == 0){
                isWin = true;
            }
        }
    }
    if(isWin == false) return;
    onWin(player);
}

void GameModel::onWin(int player)
{
    QString winText[2] = {"黑棋获胜","白棋获胜"};
    state = GameOver;
    QMessageBox::information(nullptr,"游戏结束",winText[2-player]);
}


int GameModel::evaluateBoard(Player AIPlay)
{
    int blackFobbidden = 0; //黑棋不能下的点数
    int whiteFobbidden = 0; //白棋不能下的点数
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(board[i][j] != 0) continue; //只有棋盘空位才可落子
            if(isAbleToPlaceChess(i,j,1) == false) blackFobbidden++;
            if(isAbleToPlaceChess(i,j,2) == false) whiteFobbidden++;
        }
    }

    //qDebug() << "黑棋不能下的点数为:" << blackFobbidden << " 白棋不能下的点数为:" << whiteFobbidden;
    //qDebug() << blackFobbidden - whiteFobbidden;

    //保证返回值越大，对AI越有利
    if(AIPlay == White) return blackFobbidden-whiteFobbidden;
    else return whiteFobbidden - blackFobbidden;
}

bool GameModel::isAbleToPlaceChess(int lx, int ly, int player)
{
    //检查是否有异色棋子气为0
    int sides[4][2] = {{-1,0},{1,0},{0,1},{0,-1}}; //表示上下左右四个方向
    for(int s = 0;s < 4;s++){
        int i = sides[s][0];
        int j = sides[s][1];
        if(board[lx+i][ly+j] == 3-player && qi[lx+i][ly+j] == 1){
            return false;
        }
    }
    //检查是否有同色棋子气为0
    bool result = false; //先默认result为false,表示不能落子
    for(int s = 0;s < 4;s++){
        int i = sides[s][0];
        int j = sides[s][1];
        if(board[lx+i][ly+j] == 0){
            result = true;
            break;
        }
        if(board[lx+i][ly+j] == player && qi[lx+i][ly+j] > 1){
            result = true;
            break;
        }
    }

    return result;
}

EvaluateValue GameModel::miniMax(int alpha,int beta,int depth,int maxDepth,Player AIPlay)
{
    //从depth = 1开始
    EvaluateValue ev;
    ev.lx = -1; //这些lx,ly值没用，随便设就行
    ev.ly = -1;
    if(depth > maxDepth){
        ev.value = evaluateBoard(AIPlay);
        return ev;
    }
    if(depth % 2 == 1) //MAX层，下棋的是AIPlay
    {
        for(int i = 1;i <= LINE_NUM;i++){
            for(int j = 1;j <= LINE_NUM;j++){
                if(board[i][j] != 0) continue;
                if(isAbleToPlaceChess(i,j,AIPlay)==false) continue;
                board[i][j] = AIPlay;
                int copyqi[20][20] = {{0}};
                for(int a = 0;a < LINE_NUM+2;a++){
                    for(int b = 0;b < LINE_NUM+2;b++){
                        copyqi[a][b] = qi[a][b];
                    }
                }
                updateQi(i,j,AIPlay);
                int value = miniMax(alpha,beta,depth+1,maxDepth,AIPlay).value;
                board[i][j] = 0;
                for(int a = 0;a < LINE_NUM+2;a++){
                    for(int b = 0;b < LINE_NUM+2;b++){
                        qi[a][b] = copyqi[a][b];
                    }
                }

                if(value > alpha){
                    alpha = value;
                    ev.lx = i;
                    ev.ly = j;

                }
                if(alpha >= beta){
                    ev.value = beta;
                    return ev;
                }
            }
        }
        ev.value = alpha;
        return ev;
    }else{ //MIN层，下棋的是3-AIPlay
        for(int i = 1;i <= LINE_NUM;i++){
            for(int j = 1;j <= LINE_NUM;j++){
                if(board[i][j] != 0) continue;
                if(isAbleToPlaceChess(i,j,3-AIPlay)==false) continue;
                board[i][j] = 3-AIPlay;
                int copyqi[20][20] = {{0}};
                for(int a = 0;a < LINE_NUM+2;a++){
                    for(int b = 0;b < LINE_NUM+2;b++){
                        copyqi[a][b] = qi[a][b];
                    }
                }
                updateQi(i,j,3-AIPlay);
                int value = miniMax(alpha,beta,depth+1,maxDepth,AIPlay).value;
                board[i][j] = 0;
                for(int a = 0;a < LINE_NUM+2;a++){
                    for(int b = 0;b < LINE_NUM+2;b++){
                        qi[a][b] = copyqi[a][b];
                    }
                }

                if(value < beta){
                    beta = value;
                    ev.lx = i;
                    ev.ly = j;

                }
                if(alpha >= beta){
                    ev.value = alpha;
                    return ev;
                }
            }
        }
        ev.value = beta;
        return ev;
    }
}

EvaluateValue GameModel::MCTS(Player AIPlay)
{
    clock_t start_time = clock(); //记录初始时刻
    //利用当前游戏状态创建根结点
    Node * root = new Node;
    for(int i = 0;i <= LINE_NUM+1;i++){
        for(int j = 0;j <= LINE_NUM+1;j++){
            root->board[i][j] = board[i][j];
        }
    }
    root->N = root->Q = 0;
    root->parent = nullptr;
    root->children.clear();
    root->player = AIPlay;
    root->generateAvailablePoints(root);
    //如果在根结点就无路可下，说明AI落败，直接返回
    if(root->available_points.empty()){
        return EvaluateValue{-1,-1,0};
    }

//    int cnt = 0;

    while(true){
        clock_t end_time = clock(); //记录结束时刻
        if((end_time - start_time)*1.0 / CLOCKS_PER_SEC > 1.95) break; //当消耗时间大于阈值时，停止模拟
        Node * node = root->treePolicy(root);
        if(node == nullptr) break; //如果treePolicy返回的是nullptr，说明模拟初始结点已经到达终局，停止模拟
        int reward = node->defaultPolicy(node,AIPlay);
        node->backUp(node,reward);

        //记录、调试用
//        cnt++;
//        if(cnt % 2000 == 0){
//            qDebug() << "进行了" << cnt << "次模拟";
//            qDebug() << "用时" << (end_time - start_time)*1.0 / CLOCKS_PER_SEC << "秒" << endl;
//        }
    }


    EvaluateValue ev;
    ev.lx = ev.ly = -1;

    Node * best_child = root->bestChild(root,0);
    //比较best_child与root的棋盘状态，找到best_child下的那一步
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(root->board[i][j] != best_child->board[i][j]){
                ev.lx = i;
                ev.ly = j;
            }
        }
    }

    ev.value = best_child->Q*1.0/best_child->N;
    qDebug() << "AI预测胜率：" << ev.value;
    root->deleteMalloc(root); //结束前，删除结点树
    return ev;
}

void Node::deleteMalloc(Node *node)
{
    for(int i = 0;i < node->children.size();i++){
        deleteMalloc(node->children[i]);
    }
    delete node;
}

bool Node::isAllExpanded(Node *node)
{
    return node->available_points.empty();
}

int Node::defaultPolicy(Node *node,int AIPlay)
{
    Node * temp = new Node;
    //temp = node;  //不能这样写！！！！！这样temp和node指向的是同一块地址，改变temp也就改变了node
    temp->player = node->player;
    for(int i = 0;i <= LINE_NUM+1;i++){
        for(int j = 0;j <= LINE_NUM;j++){
            temp->board[i][j] = node->board[i][j];
        }
    }
    //用随机下法在当前node棋局状态下模拟一盘游戏
    while(true){
        generateAvailablePoints(temp);
        if(temp->available_points.empty()) break; //没有可下的点了，说明已到了终局，跳出游戏循环
        srand((unsigned)time(NULL));
        int random = rand() % temp->available_points.size();
        Point point = temp->available_points[random];
        temp->board[point.x][point.y] = temp->player;
        temp->player = 3 - temp->player; //交换棋子颜色
    }
    int reward = -1;
    if(temp->player == AIPlay){ //AI输
        reward = 0;
    }else{ //AI赢
        reward = 1;
    }
    delete temp;
    return reward;
}

void Node::backUp(Node *node, int reward)
{
    if(node == nullptr) return;
    node->N ++;
    node->Q += reward;
    backUp(node->parent,reward);
}

bool Node::isAvailable(Node *node, Point point)
{
    //判断在当前node棋局下，能不能再在point处下一步棋
    if(node->board[point.x][point.y] != 0) return false; //若在当前点有子，肯定不能下
    //重置dfs_visited数组
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            node->dfs_visited[i][j] = 0;
        }
    }
    int x = point.x;
    int y = point.y;
    int sides[4][2] = {{-1,0},{1,0},{0,1},{0,-1}}; //表示上下左右四个方向
    //模拟该步落子
    node->board[point.x][point.y] = node->player;
    //判断是否自杀
    if(!DFS(node,point)){
        node->board[point.x][point.y] = 0;
        return false;
    }
    //判断是否将别人气变为0
    for(int s = 0;s < 4;s++){
        Point p{x+sides[s][0],y+sides[s][1]};
        if(node->board[p.x][p.y] != 3-node->player) continue;
        //重置dfs_visited数组
        for(int i = 1;i <= LINE_NUM;i++){
            for(int j = 1;j <= LINE_NUM;j++){
                node->dfs_visited[i][j] = 0;
            }
        }
        if(!DFS(node,p)){
            node->board[point.x][point.y] = 0;
            return false;
        }
    }
    node->board[point.x][point.y] = 0; //在返回前，不要忘了取消模拟的落子
    return true;
}

bool Node::DFS(Node *node, Point point)
{
    int sides[4][2] = {{-1,0},{1,0},{0,1},{0,-1}}; //表示上下左右四个方向
    int x = point.x;
    int y = point.y;
    for(int i = 0;i < 4;i++){
        Point p{x+sides[i][0],y+sides[i][1]};
        //如果周围有在棋盘范围内的空位，则肯定是有气的
        if(node->board[p.x][p.y] == 0 && inBoard(p)){
            return true;
        }
    }
    node->dfs_visited[x][y] = 1;
    for(int i = 0;i < 4;i++){
        Point p{x+sides[i][0],y+sides[i][1]};
        if(node->board[p.x][p.y] == node->board[x][y] && !node->dfs_visited[p.x][p.y]){ //同色且没有被访问过
            if(DFS(node,p)){
                return true;
            }
        }
    }
    return false;
}

bool Node::inBoard(Point point)
{
    if(point.x > 0 && point.x <= LINE_NUM && point.y > 0 && point.y <= LINE_NUM){
        return true;
    }
    return false;
}

bool Node::isGameOver(Node *node)
{
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(isAvailable(node,Point{i,j})) return true;
        }
    }
    return false;
}

void Node::generateAvailablePoints(Node *node)
{
    node->available_points.clear();
    for(int i = 1;i <= LINE_NUM;i++){
        for(int j = 1;j <= LINE_NUM;j++){
            if(node->board[i][j] != 0) continue;
            Point point{i,j};
            if(isAvailable(node,point)){
                node->available_points.push_back(point);
            }
        }
    }
}


Node *Node::treePolicy(Node *node)
{
    if(node == nullptr) return nullptr;

    Node * best_node = nullptr;
    if(isAllExpanded(node)){ //如果该结点所有可能下法已被遍历完
        best_node = treePolicy(bestChild(node,1));
    }else{
        Point p = node->available_points.front();
        node->available_points.pop_front();
        best_node = expand(node,p);
    }
    return best_node;
}

Node *Node::expand(Node *node,Point point)
{
    Node * new_node = new Node;
    new_node->N = 0;
    new_node->Q = 0;
    new_node->parent = node;
    new_node->children.clear();
    node->children.push_back(new_node);
    new_node->player = 3 - node->player;
    for(int i = 0;i <= LINE_NUM+1;i++){
        for(int j = 0;j <= LINE_NUM+1;j++){
            new_node->board[i][j] = node->board[i][j];
        }
    }
    new_node->board[point.x][point.y] = node->player;
    new_node->generateAvailablePoints(new_node);
    return new_node;
}

Node *Node::bestChild(Node *node,double c)
{
    Node * best_child = nullptr;
    double best_point = -2e20;
    for(int i = 0;i < node->children.size();i++){
        Node * child = node->children[i];
        double point = child->Q*1.0/child->N + c * sqrt(2*log(node->N)/child->N);
        if(point > best_point){
            best_child = child;
            best_point = point;
        }
    }
    return best_child;
}




