#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <fstream>
#include <sstream>
#include <QScrollBar>
#include <codeeditor.h>
#include "mi_gdb.h"
#include <vector>
#include <fstream>
#include <QMessageBox>
#include <cstdlib>

#define TryRun(a,b) if (!DoTryRun(a,b)) return
extern int readcommands(char*,int);

std::vector<int> breakpoints;
std::vector<int> tempbreakpoints;

std::vector<mi_bkpt*> gdbbreakpoints;

int firstline;



mi_aux_term *child_vt=NULL;
mi_bkpt *bk;
mi_bkpt *fist_bk;
mi_wp *wp;
mi_frames *fr;
int fistbreakpointline = 0;
/* This is like a file-handle for fopen.
    Here we have all the state of gdb "connection". */
mi_h *h;

int curline = 0;

bool isDebugging = false;
QString path;
QString status = "";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    editor = new CodeEditor(ui->plainTextEdit);
    editor->show();

    setWindowState(Qt::WindowMaximized);
    createActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initialize(){
    ui->statusBar->hide();
    ui->label->setAlignment(Qt::AlignCenter);

    //    CodeEditor editor;// = new CodeEditor(this);
    //    editor.setWindowTitle(QObject::tr("Code Editor Example"));
    //    editor.show();
    //    setCentralWidget(editor);


    qDebug()<<ui->plainTextEdit->width();
    qDebug()<<ui->plainTextEdit->height();
    editor->resize(ui->plainTextEdit->width(),ui->plainTextEdit->height());

}

void MainWindow::createActions()
{
    //  QList menu_file_actions = ui->menu_File->actions();

    //    QAction *newAction = ui->menu_File->findChild("Open");
    //    newAction->setShortcut( tr("Ctrl+N") );
    //    newAction->setStatusTip( tr("Create a new document") );

    //    QAction *action = ui->menu_File->actionOpen;
    //    action->

    connect(ui->menu_File, SIGNAL(triggered(QAction*)), this, SLOT(open(QAction*)));

    connect(ui->menu_Debug, SIGNAL(triggered(QAction*)), this, SLOT(debug(QAction*)));

    connect(ui->menuDebugESI, SIGNAL(triggered(QAction*)), this, SLOT(debugESI(QAction*)));




    //  QAction *newAction = new QAction( QIcon(":/images/new.png"), tr("&New"), this );
    //  newAction->setShortcut( tr("Ctrl+N") );
    //  newAction->setStatusTip( tr("Create a new document") );
    //  connect( newAction, SIGNAL(triggered()), this, SLOT(fileNew()) );

}

void  setBreakPointInGDB(int linenubmer){
    breakpoints.push_back(linenubmer);
}


void cb_console(const char *str, void *data)
{
    printf("CONSOLE> %s\n",str);
}

/* Note that unlike what's documented in gdb docs it isn't usable. */
void cb_target(const char *str, void *data)
{
    printf("TARGET> %s\n",str);
}

void cb_log(const char *str, void *data)
{
    printf("LOG> %s\n",str);
}

void cb_to(const char *str, void *data)
{
    printf(">> %s",str);
}

void cb_from(const char *str, void *data)
{
    printf("<< %s\n",str);
}

volatile int async_c=0;

void cb_async(mi_output *o, void *data)
{
    printf("ASYNC\n");
    async_c++;
}

int wait_for_stop(mi_h *h)
{
    int res=1;
    mi_stop *sr;

    while (!mi_get_response(h))
        usleep(1000);
    /* The end of the async. */
    sr=mi_res_stop(h);
    if (sr)
    {
        printf("Stopped, reason: %s\n",mi_reason_enum_to_str(sr->reason));
        mi_free_stop(sr);
        curline = sr->frame->line;
    }
    else
    {
        printf("Error while waiting\n");
        res=0;
    }
    return res;
}

void print_frames(mi_frames *f, int free_f)
{
    mi_frames *ff=f;

    if (!f)
    {
        printf("Error! empty frames info\n");
        return;
    }
    while (f)
    {
        printf("Level %d, addr %p, func %s, where: %s:%d args? %c\n",f->level,f->addr,
               f->func,f->file,f->line,f->args ? 'y' : 'n');
        f=f->next;
    }
    if (free_f)
        mi_free_frames(ff);
}


void print_gvar(mi_gvar *v)
{
    if (!v)
    {
        printf("Error! failed to define variable\n");
        return;
    }
    printf("Variable name: '%s', type: '%s', number of children: %d format: %s expression: %s lang: %s editable: %c\n",
           v->name,v->type,v->numchild,mi_format_enum_to_str(v->format),
           v->exp,mi_lang_enum_to_str(v->lang),v->attr & MI_ATTR_EDITABLE ? 'y' : 'n');
}

void MainWindow::setBreakPoint(){
    /* Set a breakpoint. */
    bk=gmi_break_insert(h,path.toStdString().c_str(),breakpoints.back());
    if (!bk)
    {
        printf("Error setting breakpoint\n");
        qDebug()<<"Error setting breakpoint\n";
        ui->status->setText(status);
        mi_disconnect(h);
        return;
    }
    printf("Breakpoint %d @ function: %s\n",bk->number,bk->func);
    ui->status->setText(status);
    qDebug()<<"Breakpoint "<<bk->number<<" @ function: "<<bk->func<< " @ line: "<< bk->line << " @ mode: "<< bk->mode << " @ thread "<<bk->thread << " @ addr: "<< bk->addr;
    qDebug()<<"T_Breakpoint "<<bk->cond;

    //    if(bk->func=="function:  main(int, char**)"){
    //        if(bk->)
    //    }

    status.append("Breakpoint ");
    status.append(QString(bk->number));
    status.append(" @ function: ");
    status.append(QString(bk->func));
    status.append(QString("\n"));

    if(bk->next){
        qDebug()<<"Next breakpoint"<<bk->next->line;
    }

    /* You can do things like:
     gmi_break_delete(h,bk->number);
     gmi_break_set_times(h,bk->number,2);
     gmi_break_set_condition(h,bk->number,"1");
     gmi_break_state(h,bk->number,0);*/
    /* If we no longer need this data we can release it. */
    //    gdbbreakpoints.push_back(bk);
    //    mi_free_bkpt(bk);
    breakpoints.pop_back();
    ui->status->setText(status);
}

int wait_for_stop(MIDebugger &d)
{
    int res=1;
    mi_stop *sr;

    while (!d.Poll(sr))
        usleep(1000);
    /* The end of the async. */
    if (sr)
    {
        printf("Stopped, reason: %s\n",mi_reason_enum_to_str(sr->reason));
        mi_free_stop(sr);
    }
    else
    {
        printf("Error while waiting\n");
        printf("mi_error: %d\nmi_error_from_gdb: %s\n",mi_error,mi_error_from_gdb);
        res=0;
    }
    return res;
}


int DoTryRun(int res, MIDebugger &d)
{
    if (!res)
    {
        printf("Error in executing!\n");
        return 0;
    }
    if (!wait_for_stop(d))
        return 0;
    return 1;
}

std::string exec(char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}


void MainWindow::debugESI(QAction* action){
    if(action->text()=="DebugESI"){
        status = "";
        if(isDebugging){
            QMessageBox::information(
                        this,
                        tr("Debugging Session is still on"),
                        tr("Please stop Debugging before debugging again") );
            status.append("Debugging Session is still on\n");
            status.append("Please stop Debugging before debugging again\n");
            return;
        }

        isDebugging = true;

        printf("Hello world");
        QString pamenvpath = ui->pamenvpath->text();
        QString pamworldpath = ui->pamworldpath->text();
        QString testfilepath = ui->testfilepath->text();
        QString testfilepath1 = ui->testfilepath->text();


//        PAMPROD="vps";
//        PAMVERS="2014.0";
//        PAMSYST="Linux";
//        PAMARCH="em64t";
//        "/home/suraj/Cuda/ESICUDA/e-et-t/dev/";
        setenv("PAMPROD","vps",1);
        setenv("PAMVERS","2014.0",1);
        setenv("PAMSYST","Linux",1);
        setenv("PAMARCH","em64t",1);
        setenv("PAMROOT","/home/suraj/Cuda/ESICUDA/e-et-t/dev/",1);


//        setenv("PAM_HOME","xxx",1);

//        qDebug()<<pamworldpath;
//        char cmd[] = "";
//        strcat(cmd, pamenvpath.append("/pamenv.sh").toStdString().c_str());
//        qDebug()<<pamworldpath;
//        qDebug()<<cmd;
//        printf(cmd);
//        readcommands(cmd,strlen(cmd));
//        qDebug()<<pamenvpath.append("/pamenv.sh").toStdString().c_str();

        QString cmd = "bash ";
        cmd.append(pamenvpath.append("/pamenv.sh"));
        system(cmd.toStdString().c_str());
        qDebug()<<pamenvpath;


        char cmd1[] ="";
//        qDebug()<<pamworldpath;

        pamworldpath.append("/pamworld -fp 1 -nt 1 \\-lic CRASHSAF ").append(testfilepath.append("/truck.pc ")).append("> ").append(testfilepath1.append("/truck.out"));
//        qDebug()<<pamworldpath;
        qDebug()<<cmd;
        readcommands(strdup(pamworldpath.toStdString().c_str()),strlen(pamworldpath.toStdString().c_str()));
//        system(pamworldpath.toStdString().c_str());

//        std::string status = exec(strdup(pamworldpath.toStdString().c_str()));
//        qDebug()<<status.c_str();
//        ui->status->setText(QString(status.c_str()));

        qDebug()<<"Done";
    }
}



void MainWindow::debug(QAction* action){
    if(action->text()=="Debug"){
        status = "";
        if(isDebugging){
            QMessageBox::information(
                        this,
                        tr("Debugging Session is still on"),
                        tr("Please stop Debugging before debugging again") );
            status.append("Debugging Session is still on\n");
            status.append("Please stop Debugging before debugging again\n");
            return;
        }
        isDebugging = true;

        std::fstream fs;
        fs.open (ui->label_2->text().toStdString().c_str(),std::ofstream::trunc|std::ofstream::out);
        fs << editor->toPlainText().toStdString().c_str();
        fs.close();


        qDebug()<<editor->toPlainText();

        path = ui->label_2->text();

        char cppcmd[] = "g++ -g ";
        char fortcmd[] = "gfortran -g ";
        char* cmd;
        if(strcmp(path.toStdString().substr(path.toStdString().length()-3,path.toStdString().length()).c_str(),"f90")==0){
            cmd = fortcmd;
        }else{
            cmd = cppcmd;
        }
        strcat(cmd, path.toStdString().c_str());


        readcommands(cmd,strlen(cmd));

        h=mi_connect_local();
        if (!h)
        {
            qDebug()<<"Connect failed\n";
            status.append("Connect failed\n");
            printf("Connect failed\n");
            ui->status->setText(status);
            return;
        }
        printf("Connected to gdb!\n");
        qDebug()<<"Connected to gdb!\n";
        status.append("Connected to gdb!\n");
        ui->status->setText(status);

        /* Set all callbacks. */
        mi_set_console_cb(h,cb_console,NULL);
        mi_set_target_cb(h,cb_target,NULL);
        mi_set_log_cb(h,cb_log,NULL);
        mi_set_async_cb(h,cb_async,NULL);
        mi_set_to_gdb_cb(h,cb_to,NULL);
        mi_set_from_gdb_cb(h,cb_from,NULL);

        /* Look for a free VT where we can run the child. */
        child_vt=gmi_look_for_free_vt();
        if (!child_vt){
            qDebug()<<"Error opening auxiliar terminal, we'll use current one.\n";
            status.append("Error opening auxillary terminal, we'll use current one.\n");
            printf("Error opening auxiliar terminal, we'll use current one.\n");
            ui->status->setText(status);
        }
        else
        {
            printf("Free VT @ %s\n",child_vt->tty);
            printf("\n\n***************************************\n");
            printf("Switch to the above mentioned terminal!\n");
            printf("***************************************\n\n\n");


            qDebug()<<"Free VT @ "<<child_vt->tty<<"\n";
            qDebug()<<"\n\n***************************************\n";
            qDebug()<<"Switch to the above mentioned terminal!\n";
            qDebug()<<"***************************************\n\n\n";

            status.append("Free VT @ ");
            status.append(child_vt->tty);
            status.append("\n\n***************************************\n");
            status.append("Switch to the above mentioned terminal!\n");
            status.append("***************************************\n\n\n");
            ui->status->setText(status);
        }

        /* Tell gdb to attach the child to a terminal. */
        if (!gmi_target_terminal(h,child_vt ? child_vt->tty : ttyname(STDIN_FILENO)))
        {
            printf("Error selecting target terminal\n");
            qDebug()<<"Error selecting target terminal\n";
            status.append("Error selecting target terminal\n");
            ui->status->setText(status);
            mi_disconnect(h);
            return;
        }

        /* Set the name of the child and the command line aguments. */
        if (!gmi_set_exec(h,"./a.out","prb1 2 prb3"))
        {
            printf("Error setting exec y args\n");
            qDebug()<<"Error setting exec y args\n";
            status.append("Error setting exec y args\n");
            ui->status->setText(status);
            mi_disconnect(h);
            return;
        }

        tempbreakpoints = breakpoints;
        int size = breakpoints.size();

        bk=gmi_break_insert_full(h,1,0,NULL,-1,-1,"main");
        firstline = bk->line-1;

        qDebug()<<"Breakpoint "<<bk->number<<" @ function: "<<bk->func<< " @ line: "<< bk->line << " @ mode: "<< bk->mode << " @ thread "<<bk->thread << " @ addr: "<< bk->addr;

        for(int i=0;i<size;i++){
            this->setBreakPoint();
        }

        editor->highlightCurrentDebugLine(firstline);
        curline = firstline;


        if (!gmi_exec_run(h))
        {
            printf("Error in run!\n");
            qDebug()<<"Error in run!\n";
            qDebug()<<mi_get_error_str();
            status.append("Error in run!\n");
            ui->status->setText(status);
            mi_disconnect(h);
            return;
        }

        //
        /* Here we should be stopped at the breakpoint. */
        if (!wait_for_stop(h))
        {
            qDebug()<<"Disconnecting...";
            status.append("Disconnecting");
            ui->status->setText(status);
            mi_disconnect(h);
            return;
        }




        //        /* Get information about the calling stack. */
        //        fr=gmi_stack_list_frames(h);
        //        printf("\nCalling stack:\n\n");
        //        print_frames(fr,1);
        //        printf("\n");
        //        fr=gmi_stack_info_frame(h);
        //        printf("\nCurrent frame:\n\n");
        //        print_frames(fr,1);
        //        printf("\n");
        //        printf("Stack depth: %d\n",gmi_stack_info_depth_get(h));
        //        gmi_stack_select_frame(h,1);
        //        fr=gmi_stack_info_frame(h);
        //        printf("\nFrame 1:\n\n");
        //        print_frames(fr,1);
        //        printf("\n");



        //        mi_gvar* gv=gmi_var_create(h,-1,"v");
        //        print_gvar(gv);
        //        gmi_var_show_format(h,gv);
        //        print_gvar(gv);
        //        gmi_var_info_num_children(h,gv);
        //        print_gvar(gv);
        //        gmi_var_info_type(h,gv);
        //        print_gvar(gv);
        //        gmi_var_info_expression(h,gv);
        //        print_gvar(gv);
        //        gmi_var_show_attributes(h,gv);
        //        print_gvar(gv);
        //        gmi_var_evaluate_expression(h,gv);
        //        qDebug()<<"The value is "<<gv->value;

    }
}

void continueDebug(){
    /* Continue execution. */
    if (!gmi_exec_continue(h))
    {
        printf("Error in continue!\n");
        status.append("Error in continue");
        mi_disconnect(h);
        return;
    }
}


void MainWindow::showDebugValue(){
    QString var = "";
    printf("Watch value");
    printf(ui->lineEdit_2->text().toStdString().c_str());
    if(ui->lineEdit_2->text()!=NULL){
        printf(ui->lineEdit_2->text().toStdString().c_str());
        var = ui->lineEdit_2->text();
    }else{
        var = "";
    }
    mi_gvar* gv=gmi_var_create(h,-1,var.toStdString().c_str());
    if(gv==NULL){
        ui->label_5->setText("Variable out of scope");
        return;
    }
    print_gvar(gv);
    gmi_var_show_format(h,gv);
    print_gvar(gv);
    gmi_var_info_num_children(h,gv);
    print_gvar(gv);
    gmi_var_info_type(h,gv);
    print_gvar(gv);
    gmi_var_info_expression(h,gv);
    print_gvar(gv);
    gmi_var_show_attributes(h,gv);
    print_gvar(gv);
    gmi_var_evaluate_expression(h,gv);
    //    qDebug()<<"The value is "<<gv->value;
    //    MainWindow::getUI()->label_5->setText(gv->value);
    if(gv->value){
        ui->label_5->setText(gv->value);
    }
}


void exitDebug(){
    gmi_gdb_exit(h);
    /* Close the connection. */
    mi_disconnect(h);
    gmi_end_aux_term(child_vt);
    isDebugging = false;
}

void MainWindow::open(QAction* action)
{
    //    qDebug()<<action->text();
    if(action->text()=="Open"){
        editor->clear();
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),"",tr("Files (*.cpp *.F95 *.f90 *.F90)"));
        std::ifstream file(fileName.toStdString().c_str());
        std::string str;
        std::string totalText;
        int count = 0;
        //        ui->widget_7->layout()->setSpacing(2);
        while (std::getline(file, str))
        {
            std::stringstream ss;
            count++;
            ss << count;
            totalText.append(str.c_str());
            totalText.append("\n");
            //ui->label_3->setText(ui->label_3->text().append(QString(ss.str().c_str()).append("\n")));
            //            QLabel *lbl = new QLabel("DebugPoint",this);
            //            lbl->setContentsMargins(0,0,0,0);
            //            ui->widget_7->layout()->addWidget(lbl);
            //            lbl->setStyleSheet("font: 9pt Courier");
            //            linenumbers.append(ss.str());
            //            linenumbers.append("\n");
        }
        //        ui->label_6->setText(linenumbers.c_str());



        editor->moveCursor(QTextCursor::End);
        editor->textCursor().insertText(QString(totalText.c_str()));
        editor->moveCursor(QTextCursor::End);

        //        editor->parentWidget()->setText("");
        //        ui->plainTextEdit->setText(QString(totalText.c_str()));
        //        ui->plainTextEdit->setDocument(new QTextDocument(QString(totalText.c_str())));

        //        ui->plainTextEdit->appendPlainText(QString(totalText.c_str()));

        //        ui->textEdit->setStyleSheet("wrapMode: TextEdit.Wrap");
        //        ui->plainTextEdit->setStyleSheet("font: 11pt Courier;");
        //        ui->textEdit->verticalScrollBar()->setMaximum(ui->scrollArea->verticalScrollBar()->maximum());
        //        ui->scrollArea->verticalScrollBar()->setDisabled(true);
        //        int maxscroll = ui->scrollArea->verticalScrollBar()->maximum();
        //        ui->scrollArea->setVerticalScrollBar(ui->plainTextEdit->verticalScrollBar());
        //        ui->scrollArea->verticalScrollBar()->setValue(maxscroll);



        //        ui->scrollArea->verticalScrollBar()->setStyleSheet(
        //            "QScrollBar:vertical { width: 10px; }");

        //        ui->scrollArea->verticalScrollBar()->setMaximum(maxscroll);
        //        ui->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        ui->label_2->setText(fileName);
        ui->label_2->setTextInteractionFlags(Qt::TextEditable);

        qDebug()<<fileName;
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    editor->resize(ui->plainTextEdit->width(),ui->plainTextEdit->height());
    // Your code here.
}

void MainWindow::on_pushButton_7_clicked()
{
    showDebugValue();
}

void MainWindow::on_pushButton_8_clicked()
{
    editor->clear();
    exitDebug();
}

void MainWindow::on_pushButton_3_clicked()
{
    qDebug()<<"Stepping";
    fr=gmi_stack_list_frames(h);
    //    printf("\nCalling stack:\n\n");
    //    print_frames(fr,1);
    //    int line = bk->line;
    if(fr){
        curline = fr->line;
    }
    qDebug()<<"line number "<<curline;
    gmi_exec_next(h);

    if (!wait_for_stop(h))
    {
        qDebug()<<"Disconnecting...";
        status.append("Disconnecting");
        ui->status->setText(status);
        mi_disconnect(h);
        return;
    }
    showDebugValue();
    editor->highlightCurrentDebugLine(curline);

}

void MainWindow::on_pushButton_9_clicked()
{
    QString pamdir = QFileDialog::getExistingDirectory(this, tr("Open pamworld Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->pamworldpath->setText(pamdir);




    //    QString var = ui->variable->text();
    //    QString value = ui->value->text();

    //    gmi_gdb_set(h,var.toStdString().c_str(),value.toStdString().c_str());
}

void MainWindow::on_pushButton_10_clicked()
{
    QString pamenvdir = QFileDialog::getExistingDirectory(this, tr("Open pamenv Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->pamenvpath->setText(pamenvdir);
}

void MainWindow::on_pushButton_11_clicked()
{
    QString testDir = QFileDialog::getExistingDirectory(this, tr("Open test Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->testfilepath->setText(testDir);
}
