// ============================================================================
// Copyright Jean-Charles LAMBERT - 2007-2008                                  
// e-mail:   Jean-Charles.Lambert@oamp.fr                                      
// address:  Dynamique des galaxies                                            
//           Laboratoire d'Astrophysique de Marseille                          
//           P�le de l'Etoile, site de Ch�teau-Gombert                         
//           38, rue Fr�d�ric Joliot-Curie                                     
//           13388 Marseille cedex 13 France                                   
//           CNRS U.M.R 6110                                                   
// ============================================================================
// See the complete license in LICENSE and/or "http://www.cecill.info".        
// ============================================================================
#include "formobjectcontrol.h"
#include <iostream>
#include <sstream>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QRegExp>
#include <assert.h>
#include "glwindow.h"
#include "globaloptions.h"
#include "gltexture.h"

namespace glnemo {
#define RT_VISIB 0
#define RT_COLOR 2
#define FT_VISIB 0
#define FT_COLOR 3

int last_row=0;
bool EMIT=true;  // EMIT only signal from user requests

// ============================================================================
// Constructor                                                                 
// create the 2 tables : Range and File                                        
FormObjectControl::FormObjectControl(QWidget *parent):QDialog(parent)
{
  ignoreCloseEvent = true;
  form.setupUi(this);
  current_data = NULL;
  form.range_table->setColumnWidth(0,25);   // Vis  
  form.range_table->setColumnWidth(1,150);  // Range
  form.range_table->setColumnWidth(2,45);   // Color
  form.file_table->setColumnWidth(0,25);
  for (int i=0; i < form.range_table->rowCount(); i++) {
    form.range_table->setRowHeight(i,25);
    form.file_table->setRowHeight(i,25);
  }
  // create object index array
  int no=form.range_table->rowCount()+form.file_table->rowCount();
  object_index = new int[no];
  for (int i=0; i<no;i++) object_index[i]=-1;
  current_object = 0;
  pov = NULL;
  first=true;
  lock = true;
  // insert checkbox and color widget into tables
  initTableWidget(form.range_table,0,RT_VISIB,RT_COLOR);  // range
  initTableWidget(form.file_table,1,FT_VISIB,FT_COLOR);   // file 
  // Selection mode and behaviour
  form.range_table->setSelectionMode(QAbstractItemView::SingleSelection);
  form.range_table->setSelectionBehavior(QAbstractItemView::SelectItems);
  form.file_table->setSelectionMode(QAbstractItemView::SingleSelection);
  form.file_table->setSelectionBehavior(QAbstractItemView::SelectItems);
  // create range selection combobox to store objects' particles indexes.
  combobox = new QComboBoxTable(0,0,0);
  form.range_table->setCellWidget(0,RT_COLOR-1,combobox);
  // add default object range
  combobox->addItem("");
  combobox->addItem("0:99999");  // add default range 1
  combobox->addItem("0:199999"); // add default range 2
  connect(combobox,SIGNAL(comboActivated(const int, const int)),this,
          SLOT(checkComboLine(const int, const int)));
  // intialyze texture combobox according to the texture array
  // loop and load all embeded textures                       
  int i=0;
  while (GLTexture::TEXTURE[i][0]!=NULL) {
    form.texture_box->addItem(QIcon(GLTexture::TEXTURE[i][0]),GLTexture::TEXTURE[i][1]);
    std::cerr << "texture i="<<i<<" = " << GLTexture::TEXTURE[i][1].toStdString() << "\n";
    i++;
  }
  // check GLSL_support
  if (! GLWindow::GLSL_support) {
    form.gaz_glsl_check->setChecked(false);
    form.gaz_glsl_check->setDisabled(true);
  } else {
    form.gaz_glsl_check->setChecked(true);
  }
  //connect(combobox,SIGNAL(my_editTextChanged(const QString&, const int, const int)),
  //      this,SLOT(updateRange(const QString&, const int, const int)));
  go = NULL;
  //form.dens_histo_view->setParent(form.tab_density);
  dens_histo = new DensityHisto(form.dens_histo_view);
  form.dens_histo_view->setScene(dens_histo);
  form.dens_glob_box->setDisabled(true);
  dens_color_bar = new DensityColorBar(go,form.dens_bar_view);
  form.dens_bar_view->setScene(dens_color_bar);
}

// ============================================================================
// Destructor                                                                  
FormObjectControl::~FormObjectControl()
{
  delete [] object_index;
  //delete combobox;
}
// ============================================================================
// initTableWidget()                                                           
// Initialize Range and File table                                             
void FormObjectControl::initTableWidget(QTableWidget * table, const int i_table,
                                        const int c_visib, const int c_col)
{
  for (int i=0; i<table->rowCount(); i++) {
#if 0
    ParticlesObject * po = new ParticlesObject();
    povrange.push_back(*po);
    delete po;
#endif
    // set checkbox widget
    QCheckBoxTable * checkbox = new QCheckBoxTable(i,i_table,this);
    assert(checkbox);
    if ( ! i_table)
      connect(checkbox,SIGNAL(changeVisib(const bool,const int ,const int )),
	      this,SLOT(updateVisib(const bool ,const int ,const int )));
    else
      connect(checkbox,SIGNAL(changeVisib(const bool,const int ,const int )),
	      this,SLOT(updateVisib(const bool ,const int ,const int )));
    checkbox->setChecked(false);
    table->setCellWidget(i,c_visib,checkbox);
    // set Color
    QTableWidgetItem  * cellcol=new QTableWidgetItem();
    cellcol->setBackground(Qt::white);
    table->setItem(i,c_col,cellcol);
    // set particles info
    QTableWidgetItem  * partcol=new QTableWidgetItem();
    partcol->setText(""); // put a blank text
    table->setItem(i,c_col-1,partcol);
  }
}
// ============================================================================
// resetTableWidget()                                                          
// RAZ Range and File table                                                    
void FormObjectControl::resetTableWidget(QTableWidget * table, const int i_table,
                                         const int c_col, const int row)
{
  // get item color
  QTableWidgetItem * cellcol = table->item(row,c_col);
  assert(cellcol);
  // set color
  cellcol->setBackground(Qt::white);
  //!!table->setItem(row,c_col,cellcol);
  // get visibility box
  QCheckBoxTable * checkbox = static_cast<QCheckBoxTable *>
  (table->cellWidget(row,i_table));
  assert(checkbox);
  // set visibility
  checkbox->setChecked(false);
  // set particles info
  QTableWidgetItem  * partrange=table->item(row,c_col-1);
  assert(partrange);
  partrange->setText(""); // put a blank text
  //!!table->setItem(row,c_col-1,partrange);
}
// ============================================================================
// update()                                                                    
// upate table with new objects                                                
void FormObjectControl::update(const ParticlesData   * _p_data,
                               ParticlesObjectVector * _pov,
                               GlobalOptions         * _go,
			       bool                    reset_table)
{

  go           = _go;
  dens_color_bar->setGo(go);
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  lock=false;
  current_data = _p_data;
  nbody        = *(current_data->nbody);
  pov          = _pov;

  
  int cpt=0;
  combobox->clear();
  for (int i=0; i < form.range_table->rowCount(); i++) {
    if (i < (int) pov->size()) { // remains objects
      object_index[i]=cpt++;
      QTableWidget * tw;
      if ((*pov)[i].selectFrom() == ParticlesObject::Range) { // Range
        tw = form.range_table;	
        updateTable(tw,i,RT_VISIB ,RT_COLOR);
        updateRangeTable(i);
      }
      else {                                                  // File
        tw = form.range_table;
        updateTable(tw,i,RT_VISIB,RT_COLOR);
        updateFileTable(i);
      }
    }
    else {               // not belonging to object list
      if (reset_table) {
	object_index[i]=-1;
	resetTableWidget(form.range_table,RT_VISIB,RT_COLOR,i);
      } else {
	//std::cerr << "combobox = " << (combobox->currentText()).toStdString() << "\n";
      }

    }
    //if (i) form.range_table->setCellWidget(i,1,NULL);

  }
  first=false;
  ///!!!!updateObjectSettings(0);
  updateObjectSettings(last_row); 
  form.range_table->setCurrentCell(current_object,1);
  // set active row
  lock=true;
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
  
  on_range_table_cellClicked(last_row,1);
}
// ============================================================================
// updateTable()                                                               
void FormObjectControl::updateTable(QTableWidget * table, const int row,
                                        const int c_visib, const int c_col)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj=object_index[row];      // object's index
  assert(i_obj < (int)pov->size());
  // update visible box
  QCheckBoxTable * checkbox = static_cast<QCheckBoxTable *>(table->cellWidget(row,c_visib));
  assert(checkbox);
  checkbox->setChecked((*pov)[i_obj].isVisible());
  // update color
  QTableWidgetItem * twi = table->item(row,c_col);
  assert(twi);
  twi->setBackground(QBrush((*pov)[i_obj].getColor())); // !!!!!!!
  //(*pov)[i_obj].setColor(twi->background().color());      // !!!!!!!
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// updateRangeTable()                                                          
void FormObjectControl::updateRangeTable(const int row)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj=object_index[row];      // object's index
  assert(i_obj < (int)pov->size());
  QString text;
  if ((*pov)[i_obj].step == 1)
    text = QString("%1:%2"   ).arg((*pov)[i_obj].first,0,10).arg((*pov)[i_obj].last,0,10);
  else
    text = QString("%1:%2:%3").arg((*pov)[i_obj].first,0,10)
	                      .arg((*pov)[i_obj].last ,0,10)
	          	      .arg((*pov)[i_obj].step ,0,10);
  QTableWidgetItem * twi = form.range_table->item(row,1);
  assert(twi);
  twi->setText(text);     // put the object range corresponding to the object to the TABLE
  combobox->addItem(text);// add the object range in the combobox
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// updateFileTable()                                                           
void FormObjectControl::updateFileTable(const int row)
{
  if (row) {;}
}
// ============================================================================
// on_range_table_cellClicked()                                                
// process the Clicked event                                                   
int FormObjectControl::on_range_table_cellClicked(int row,int column)
{
  if (! pov) return 0;
  //if (lock)
  if (go  && ! go->duplicate_mem)
    mutex_data->lock();
  lock=false;

  bool valid=false;
  last_row = row; // save the current row clicked
  int i_obj= object_index[row]; // object's index
  //assert(i_obj < (int)pov->size());
  // -
  // --- process click event on [combo box cell]
  // -
  int rcolumn=1;
  // get the combobox from the active cell
  QComboBoxTable * my_combo = static_cast<QComboBoxTable *>
                        (form.range_table->cellWidget(row,rcolumn));

  // get back the text line from cell
  QTableWidgetItem *  item = (form.range_table->item(row,rcolumn)); // check item in range column !!
  assert(item);
  QString text="none";
  text=item->text();

  if ( ! my_combo ) { // no combobox yet from the cell
    if (form.range_table->cellWidget(combobox->row,rcolumn)) {
      QComboBoxTable * combobox1 = new QComboBoxTable(row,rcolumn,this); // new combo
      *combobox1 = *combobox;                                    // copy old to new
      form.range_table->removeCellWidget(combobox->row,rcolumn); // remove old
      delete combobox;
      combobox   = combobox1;                                    // link old to new
                                                                 // establish connection
      connect(combobox,
              SIGNAL(comboActivated(
                                   const int, const int)),this,
              SLOT(checkComboLine(const int, const int)));
    }
    combobox->row = row;                                         // adjust row
    combobox->display();
    combobox->setEditText(item->text());                         // add object to the combobox
    form.range_table->setCellWidget(row,rcolumn,combobox);       // put the combobox to the current cell
  }
  else {              // widget exist
    assert(combobox->row == row);
//    combobox->setEditText(item->text()); // add object to the combobox
//    combobox->display();
#if 0
    combobox->setEditText(item->text()); // add object to the combobox
    combobox->display();
#else
    if (object_index[row] != -1 )          // object exist        
      combobox->setEditText(item->text()); // put range from table
    combobox->display();                   // display combobox    
#endif 
  }

  current_object = row;
  updateObjectSettings(row);   // Update object properties on the form
  
  // -
  // --- process click event on [Color cell]
  // -
  if (column == RT_COLOR) {
    form.range_table->setCurrentCell(row,column-1);
    // update color
    QTableWidgetItem * twi = form.range_table->item(row,column);
    assert(twi);
    QColor color=QColorDialog::getColor(twi->background().color());
    if (color.isValid() && pov && i_obj != -1) {
      valid=true;
      twi->setBackground(QBrush(color));
      ParticlesObject * pobj = &(*pov)[i_obj];
      pobj->setColor(color);
      emit gazColorObjectChanged(i_obj);
    }
  }
  if (column == RT_VISIB) {

  }
  current_object = row;

  if (valid) {
    if (i_obj < (int)pov->size()) {
      //emit gazAlphaObjectChanged(i_obj);
    }
    emit objectSettingsChanged();
  }
  lock=true;
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
  return 1;
}
// ============================================================================
// checkComboLine()                                                            
// parse the range entered in the combobox. Create a new object if valid       
void FormObjectControl::checkComboLine(const int row, const int col)
{
  //if (lock)
  if (go  &&  ! go->duplicate_mem) mutex_data->lock();
  lock=false;
  QTableWidgetItem *  item = (form.range_table->item(row,col));
  assert(item);
  if (item) {
    //construct regexp
    QRegExp rx("^(\\d{1,})((:)(\\d{1,})){,1}((:)(\\d{1,})){,1}$");
    int match=rx.indexIn(combobox->currentText());
    if (match == -1) { // not match
    }
    else {
      int first,last,step=1;
      // get first
      std::istringstream iss((rx.cap(1)).toStdString());
      iss >> first;
      // get last
      if (rx.numCaptures()>4) {
        std::istringstream  iss((rx.cap(4)).toStdString());
        iss >> last;
        // get step
        if (rx.numCaptures()>=7) {
          std::istringstream  iss((rx.cap(7)).toStdString());
          iss >> step;
        }
      }
      else {
      last=first;
      }
#if 0
      std::cerr << "whole string =["<<(rx.cap(0)).toStdString()<<"\n";
      for (int i=0;i<=rx.numCaptures();i++) {
        std::cerr << "cap ="<<(rx.cap(i)).toStdString()<<"\n";
      }
      std::cerr << "ncap="<<rx.numCaptures()<<" first="<<first<<" last="<<last<<" step="<<step<<"\n";
#endif
      // check if the syntax is correct
      int npart=last-first+1; // #part 
//      if (current_data && npart <= *(current_data->nbody) && npart>0) {    // valid object
      if (pov && npart <= nbody && npart>0) {    // valid object
        item->setText(combobox->currentText()); // fill cell
        int i_obj= object_index[row];      // object's index

        if ( i_obj != -1) { // it's an existing object
          assert(i_obj < (int)pov->size());
          ParticlesObject * pobj = &(*pov)[i_obj];
          pobj->npart = npart;
          pobj->first = first;
          pobj->last  = last;
          pobj->step  = step;
          pobj->buildIndexList();   // build object index list
          current_object = row;
          updateObjectSettings(row);// update form !!! CAUSE OF CRASH
          emit objectUpdate();      // update OpenGL
        }
        else {              // it's a new object
          ParticlesObject * po = new ParticlesObject(); // new object                
          po->buildIndexList( npart,first,last,step);   // object's particles indexes
          // get color
          QTableWidgetItem * twi = form.range_table->item(row,RT_COLOR);
          assert(twi);
          // set color 
          po->setColor(twi->background().color());
          // get visibility 
          QCheckBoxTable * checkbox = static_cast<QCheckBoxTable *>
          (form.range_table->cellWidget(row,RT_VISIB));
          assert(checkbox);
          // set visibility
          po->setVisible(checkbox->isChecked());
          pov->push_back(*po);                    // insert object
          delete po;
          object_index[row] = pov->size()-1; // update object list
          current_object = row;              //                  
          updateObjectSettings(row);         // update form !!! CAUSE OF CRASH
          emit objectUpdate();               // update OpenGL object
        }
      }
      else {   // invalid object
      }
      // check if the syntax is correct
    }
  }
  else {
    std::cerr << "WARNING [checkComboLine] no item !!!!\n";
  }
  lock=true;
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_coon_commit_button_clicked()                                             
void FormObjectControl::on_range_table_cellPressed(int row,int column)
{
    if (row && column) {;}
//  std::cerr << "cell cellPressed : " << row << " X " << column << "\n";
}
// ============================================================================
// on_coon_commit_button_clicked()                                             
void FormObjectControl::on_range_table_itemClicked(QTableWidgetItem * item)
{
    if (item) {;}
//  std::cerr << "item Clicked : ";
}
// ============================================================================
// updateVisib()                                                               
void FormObjectControl::updateVisib(const bool visib,const int row,const int table)
{
  if (table) {;}
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj= object_index[row]; // object's index
  if (pov && i_obj != -1) {
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setVisible(visib);
    emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// updateRange()                                                               
void FormObjectControl::updateRange(const QString& text, const int row, const int table)
{
  if (text=="") {;}
  if (table) {;}
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj= object_index[row]; // object's index
  if (pov && i_obj != -1) {
    assert(i_obj < (int)pov->size());
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// updateObjectSettings()                                                      
// update all the properties of the object in the form (slides, buttons etc..) 
void FormObjectControl::updateObjectSettings( const int row)
{
  //if (go  && ! go->duplicate_mem) mutex_data->lock();
  my_mutex.lock();
  EMIT = false;
  int i_obj= object_index[row]; // object's index
  if (pov && i_obj != -1) {
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    // Particles Settings
    form.part_check->setChecked(pobj->isPartEnable());
    form.part_slide_size->setValue((int) (pobj->getPartSize()*form.part_slide_size->maximum()/
                                   GlobalOptions::MAX_PARTICLES_SIZE));
    form.part_slide_alpha->setValue(pobj->getPartAlpha());
    // Gaz Settings
    form.gaz_check->setChecked(pobj->isGazEnable());
    form.gaz_slide_size->setValue((int) (pobj->getGazSize()*form.gaz_slide_size->maximum()/
                                   pobj->getGazSizeMax()));
    form.gaz_slide_alpha->setValue(pobj->getGazAlpha());
    form.texture_spin->setValue(pobj->getGazSizeMax());
    form.gaz_rot_check->setChecked(pobj->isGazRotate());
    if (GLWindow::GLSL_support)
      form.gaz_glsl_check->setChecked(pobj->isGazGlsl());
    form.texture_box->setCurrentIndex(pobj->getTextureIndex());
    // Velocity Settings
    form.vel_check->setChecked(pobj->isVelEnable());
    int x=(float) pobj->getVelSize() * (float)
                     form.vel_slide_size->maximum()/pobj->getVelSizeMax();
    form.vel_slide_size->setValue(x);
    form.vel_slide_alpha->setValue(pobj->getVelAlpha());
    form.vel_spin->setValue((int) (pobj->getVelSizeMax()));
    // -- Orbits TAB
    form.odisplay_check->setChecked(pobj->isOrbitsEnable());
    form.orecord_check->setChecked(pobj->isOrbitsRecording());
    form.orbit_history_spin->setValue(pobj->getOrbitsHistory());
    form.orbit_max_spin->setValue(pobj->getOrbitsMax());
    // -- Density TAB
    if (current_data->rho) {
      dens_histo->drawDensity(current_data->density_histo);
      float diff_rho=(log(current_data->getMaxRho())-log(current_data->getMinRho()))/100.;
      form.dens_slide_min->setValue((log(pobj->getMinDensity())-log(current_data->getMinRho()))*1./diff_rho);
      form.dens_slide_max->setValue((log(pobj->getMaxDensity())-log(current_data->getMinRho()))*1./diff_rho);
      dens_histo->drawDensity(form.dens_slide_min->value(),form.dens_slide_max->value());
      dens_color_bar->draw(form.dens_slide_min->value(),form.dens_slide_max->value());

    }
                                       
  }
  if ( i_obj == -1 ) { // no object selected
    // Particles Settings
    form.part_check->setChecked(true);
    form.part_slide_size->setValue(form.part_slide_size->maximum());
    form.part_slide_alpha->setValue(254);
    // Gaz Settings
    form.gaz_check->setChecked(false);
    form.gaz_slide_size->setValue(form.gaz_slide_size->maximum());
    form.gaz_slide_alpha->setValue(254);
    form.gaz_rot_check->setChecked(true);
    form.texture_spin->setValue(1.0);
    // Velocity Settings
    form.vel_check->setChecked(false);
    form.vel_slide_size->setValue(form.vel_slide_size->maximum());
    form.vel_slide_alpha->setValue(254);
    form.vel_spin->setValue(4);
    // -- Orbits TAB
    form.odisplay_check->setChecked(false);
    form.orecord_check->setChecked(false);
    form.orbit_history_spin->setValue(form.orbit_history_spin->maximum());
    form.orbit_max_spin->setValue(form.orbit_max_spin->maximum());
    
  }
  EMIT = true;
  //if (go  && ! go->duplicate_mem) mutex_data->unlock();
  my_mutex.unlock();
}
// ============================================================================
// checkVisib()                                                                
void FormObjectControl::checkVisib()
{
}
// ============================================================================
// ON PARTICLES                                                                
// ============================================================================

// ============================================================================
// on_part_check_clicked()                                                     
void FormObjectControl::on_part_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setPart(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_part_slide_size_valueChanged                                             
void FormObjectControl::on_part_slide_size_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setPartSize((float)value*GlobalOptions::MAX_PARTICLES_SIZE/
                             form.part_slide_size->maximum());
    //std::cerr << "part value = " << value << "\n";
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_part_slide_alpha_valueChanged                                            
void FormObjectControl::on_part_slide_alpha_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setPartAlpha(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// ON GAZ                                                                      
// ============================================================================

// ============================================================================
// on_gaz_check_clicked()
void FormObjectControl::on_gaz_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //form.texture_spin->setValue(pobj->getGazSizeMax())
    //form.gaz_slide_size->setValue(pobj->getGazSize());
    pobj->setGaz(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_gaz_slide_size_valueChanged
void FormObjectControl::on_gaz_slide_size_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setGazSize(value*form.texture_spin->value()/
                             form.gaz_slide_size->maximum());
    if (EMIT) {
      //emit gazSizeObjectChanged(i_obj);
      emit objectSettingsChanged();
    }
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_gaz_slide_alpha_valueChanged
void FormObjectControl::on_gaz_slide_alpha_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setGazAlpha(value);
    if (EMIT) {
      //emit gazAlphaObjectChanged(i_obj);
      emit objectSettingsChanged();
    }
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_gaz_rot_check_clicked()                                                  
void FormObjectControl::on_gaz_rot_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setGazRotate(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_gaz_glsl_check_clicked()
void FormObjectControl::on_gaz_glsl_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //form.texture_spin->setValue(pobj->getGazSizeMax())
    //form.gaz_slide_size->setValue(pobj->getGazSize());
    pobj->setGazGlsl(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_texture_spin_valueChanged
void FormObjectControl::on_texture_spin_valueChanged(double value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //go->vel_vector_size = form.vel_slide_size->value();
    pobj->setGazSizeMax(value);
    pobj->setGazSize((float) form.gaz_slide_size->value()*(value/form.gaz_slide_size->maximum()));
    if (EMIT) {
      //emit gazSizeObjectChanged(i_obj);
      emit objectSettingsChanged();
    }
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// ON VEL                                                                      
// ============================================================================

// ============================================================================
// on_vel_check_clicked()
void FormObjectControl::on_vel_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //pobj->setVelSize(((float) form.vel_spin->value()));
    pobj->setVel(value);
    if (EMIT) emit objectUpdateVel(i_obj);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_vel_slide_size_valueChanged
void FormObjectControl::on_vel_slide_size_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setVelSize((float) value*((float) form.vel_spin->value()/float(form.vel_slide_size->maximum())));
    if (EMIT) emit objectUpdateVel(i_obj);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_vel_slide_alpha_valueChanged
void FormObjectControl::on_vel_slide_alpha_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setVelAlpha(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_vel_spin_valueChanged                                                    
void FormObjectControl::on_vel_spin_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //go->vel_vector_size = form.vel_slide_size->value();
    pobj->setVelSizeMax(value);
    pobj->setVelSize((float) form.vel_slide_size->value()*((float) value/form.vel_slide_size->maximum()));
    if (EMIT) emit objectUpdateVel(i_obj);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// ON TEXTURE BOX                                                              
// ============================================================================

// ============================================================================
// on_texture_box_activated                                                    
void FormObjectControl::on_texture_box_activated(int index)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setTextureIndex(index);
    if (EMIT) emit textureObjectChanged(index,i_obj);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// ON Orbits TAB                                                               
// ============================================================================

// ============================================================================
// on_orecord_check_clicked
void FormObjectControl::on_orecord_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setOrbitsRecord(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}

// ============================================================================
// on_odisplay_check_ckicked                                                   
void FormObjectControl::on_odisplay_check_clicked(bool value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    pobj->setOrbits(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_orbit_max_spin_valueChanged                                                    
void FormObjectControl::on_orbit_max_spin_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //go->vel_vector_size = form.vel_slide_size->value();
    pobj->setOrbitsMax(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_orbit_history_spin_valueChanged                                          
void FormObjectControl::on_orbit_history_spin_valueChanged(int value)
{
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 ) {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    //go->vel_vector_size = form.vel_slide_size->value();
    pobj->setOrbitsHistory(value);
    if (EMIT) emit objectSettingsChanged();
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// ON DENSITY TAB                                                              
// ============================================================================

// ============================================================================
// on_dens_slide_min_valueChanged                                              
void FormObjectControl::on_dens_slide_min_valueChanged(int value)
{
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 && current_data->rho)  {  // at least one object
    assert(i_obj < (int)pov->size());
    //ParticlesObject * pobj = &(*pov)[i_obj];
    if (value >= form.dens_slide_max->value()) { // min < max !
      form.dens_slide_max->setValue(value+1);
    }
    dens_histo->drawDensity(form.dens_slide_min->value(), form.dens_slide_max->value());
    dens_color_bar->draw(form.dens_slide_min->value(), form.dens_slide_max->value());
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_dens_slide_max_valueChanged                                              
void FormObjectControl::on_dens_slide_max_valueChanged(int value)
{
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 && current_data->rho)  {  // at least one object
    assert(i_obj < (int)pov->size());
    //ParticlesObject * pobj = &(*pov)[i_obj];
    if (value <= form.dens_slide_min->value()) { // min < max !
      form.dens_slide_min->setValue(value-1);
    }
    dens_histo->drawDensity(form.dens_slide_min->value(), form.dens_slide_max->value());
    dens_color_bar->draw(form.dens_slide_min->value(), form.dens_slide_max->value());
  }
  //if (lock)
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_dens_apply_button_clicked()                                              
void FormObjectControl::on_dens_apply_button_clicked()
{
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 && current_data->rho)  {  // at least one object
    assert(i_obj < (int)pov->size());
    ParticlesObject * pobj = &(*pov)[i_obj];
    if (form.dens_loc_button->isChecked()) {
      go->dens_local = true;
      float diff_rho=(log(current_data->getMaxRho())-log(current_data->getMinRho()))/100.;
      pobj->setMinDensity(exp(log(current_data->getMinRho())+form.dens_slide_min->value()*diff_rho));
      pobj->setMaxDensity(exp(log(current_data->getMinRho())+form.dens_slide_max->value()*diff_rho));
      std::cerr << ">> slide min=" << (log(pobj->getMinDensity())-log(current_data->getMinRho()))*1/diff_rho << "\n";
      if (current_data->temp) {
        float diff_temp=(log(current_data->getMaxTemp())-log(current_data->getMinTemp()))/100.;
        pobj->setMinTemperature(exp(log(current_data->getMinTemp())+form.dens_slide_min->value()*diff_temp));
        pobj->setMaxTemperature(exp(log(current_data->getMinTemp())+form.dens_slide_max->value()*diff_temp));
      }
    } else {
      go->dens_local = false;
      go->dens_min_glob = (form.dens_min_user->text()).toFloat();
      go->dens_max_glob = (form.dens_max_user->text()).toFloat();
    }
    if (EMIT) {
      emit densityProfileObjectChanged(i_obj);
      emit objectSettingsChanged();
    }
  }
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
// ============================================================================
// on_dens_set_uselect_clicked()                                              
void FormObjectControl::on_dens_set_uselect_clicked()
{
  if (go  && ! go->duplicate_mem) mutex_data->lock();
  int i_obj = object_index[current_object];
  if (pov && pov->size()>0 && i_obj != -1 && current_data->rho)  {  // at least one object
    assert(i_obj < (int)pov->size());
    //ParticlesObject * pobj = &(*pov)[i_obj];
    float diff_rho=(log(current_data->getMaxRho())-log(current_data->getMinRho()))/100.;
    QString value;
    value.setNum(exp(log(current_data->getMinRho())+form.dens_slide_min->value()*diff_rho));
    form.dens_min_user->setText(value);
    value.setNum(exp(log(current_data->getMinRho())+form.dens_slide_max->value()*diff_rho));
    form.dens_max_user->setText(value);
    go->dens_min_glob = (form.dens_min_user->text()).toFloat();
    go->dens_max_glob = (form.dens_max_user->text()).toFloat();
  }
  if (go  && ! go->duplicate_mem) mutex_data->unlock();
}
}