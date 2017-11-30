#pragma once
#include<functional>
#include<string>
#include<map>
#include<garden/range.tcc>
#include<garden/show.tcc>

namespace garden::repl
{
  struct tokenize_fn
  : Fn<tokenize_fn>
  {
    static auto eval
    (std::string input) -> std::tuple<
      std::string,
      std::vector<std::string>
    >
    {
      using namespace range;

      return input
      ^range::from_container
      ^split( ' ' )
      ^transform(
        to_container<std::basic_string>
      )
      ^(
        maybe_front >> or_default
        & drop( 1 ) >> to_container<
          std::vector
        >
      );
    };
  };
  let tokenize = tokenize_fn{};

  using Command = std::function<std::string(std::vector<std::string>&)>;

  struct exit_loop{};
  struct show_help{};

  void main(std::map<std::string, Command> commands)
  {
    for(std::string input;
      show( "> " ),
      std::getline( std::cin, input );
    )
    {
      auto[ cmd, args ] = input^tokenize;

      if( commands^contains( cmd ) )
      {
        try
        { show( commands.at( cmd )( args ), '\n' ); }
        catch( exit_loop )
        { break; }
        catch( show_help )
        {
          show( "commands:", '\n' );

          for( auto[ id, cmd ] : commands )
            show( '\t', id, '\n' );
        };
      }
      else
      {
        show( "unrecognized command (try 'help')\n" );
      }

      std::flush( std::cout );
    }
  }
}
