library ieee ;
    use ieee.std_logic_1164.all ;
    use ieee.numeric_std.all ;
    use ieee.math_real.all ;
   
entity trigger_dual_tb is
end entity;

architecture arch of trigger_dual_tb is
    type trigger_conn is record
      armed        : std_logic;
      fired        : std_logic;
      master       : std_logic;
      trigger_in   : std_logic;
      signal_in    : std_logic;
      trigger_out  : std_logic;
      signal_out   : std_logic;
    end record;
    
    signal master : trigger_conn;
    signal slave  : trigger_conn;
    
begin
  slave.trigger_in <= master.trigger_out;
  master.trigger_in <= master.trigger_out;
  
  trig_master : entity work.trigger(async)
    generic map (
      DEFAULT_OUTPUT => '0'
    ) port map (
      armed => master.armed,
      fired => master.fired,
      master => master.master,
      trigger_in => master.trigger_in,
      signal_in => master.signal_in,
      trigger_out => master.trigger_out,
      signal_out => master.signal_out
    );

  trig_slave : entity work.trigger(async)
    generic map (
      DEFAULT_OUTPUT => '0'
    ) port map (
      armed => slave.armed,
      fired => slave.fired,
      master => slave.master,
      trigger_in => slave.trigger_in,
      signal_in => slave.signal_in,
      trigger_out => slave.trigger_out,
      signal_out => slave.signal_out
    );
    
        
    tb : process
    begin
      master.master <= '0';
      master.armed <= '0';
      master.fired <= '0';
      master.signal_in <= '1';
      slave.master <= '0';
      slave.armed <= '0';
      slave.fired <= '0';
      slave.signal_in <= '1';
      
      wait for 1 ns;
      master.master <= '1';
      
      wait for 1 ns;
      master.armed <= '1';
      slave.armed <= '1';
      
      wait for 1 ns;
      slave.fired <= '1';
      
      wait for 1 ns;
      slave.fired <= '0';
      
      wait for 1 ns;
      master.fired <= '1';
      
      wait for 1 ns;
      slave.armed <= '0';
      master.armed <= '0';
      
      wait for 1 ns;
      master.fired <= '0';
      
      wait;
    end process;
end architecture;