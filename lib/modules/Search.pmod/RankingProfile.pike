static array(int) field_ranking;
static array(int) proximity_ranking;
static int cutoff;

void create(void|int _cutoff, void|array(int) _proximity_ranking,
	    void|Search.Database.Base db, void|mapping(string:int) _field_ranking)
{
  field_ranking=allocate(66);

  // Set cutoff to a value > 0.
  cutoff = _cutoff || 8;

  proximity_ranking=allocate(8);
  if(_proximity_ranking)
    proximity_ranking = _proximity_ranking;
  else
    for(int i=0; i<8; i++)
      proximity_ranking[i]=8-i;

  if(_field_ranking)
  {
    int field_id;
    foreach(indices(_field_ranking), string field)
      if(field_id=db->get_field_id(field, 1))
	field_ranking[field_id]=_field_ranking[field];
  }
  else {
    field_ranking[0]=17;
    field_ranking[2]=147;
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
