array(int) field_ranking;
array(int) proximity_ranking;
int cutoff;

void create(void|Search.Database.Base db, void|mapping(string:int) _field_ranking)
{
  field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=147;

  proximity_ranking=allocate(8);
  for(int i=0; i<8; i++)
    proximity_ranking[i]=8-i;

  int field_id;
  if(_field_ranking)
  {
    for(int i=0; i<66; i++)
      field_ranking[i]=0;
    foreach(indices(_field_ranking), string field)
      if(field_id=db->get_field_id(field, 1))
	field_ranking[field_id]=_field_ranking[field];
  }
}

this_program copy()
{
  this_program c = this_program();
  c->field_ranking = field_ranking;
  c->proximity_ranking = proximity_ranking;
  c->cutoff = cutoff;
  return c;
}
