/*
  Author: Pontus Östlund <https://profiles.google.com/poppanator>

  Permission to copy, modify, and distribute this source for any legal
  purpose granted as long as my name is still attached to it. More
  specifically, the GPL, LGPL and MPL licenses apply to this software.
*/

//! Internal class ment to be inherited by other Google API's
class Api
{
  inherit WebApi.Api : parent;

  // Just a convenience class
  protected class Method
  {
    inherit WebApi.Api.Method;

    //! Internal convenience method
    protected mixed _get(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::get(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    protected mixed _post(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::post(get_uri(METHOD_PATH + s), p, 0, cb);
    }

    //! Internal convenience method
    protected mixed _delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::delete(get_uri(METHOD_PATH + s), p, cb);
    }
  }
}
